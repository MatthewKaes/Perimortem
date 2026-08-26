// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/language/parser/import.hpp"

#include "ttx/lexical/lexicon.hpp"

using namespace Perimortem::Core;
using namespace Ttx::Concept;
using namespace Ttx::Lexical;
using namespace Tetrodotoxin::Language;
using Perimortem::System::Version;

static auto is_package_import(const Cursor& cursor, Count offset) -> Bool {
  return cursor.peek(S64(offset)).get_code() == Code::Type::Package;
}

auto Parser::Import::is_next(const Cursor& cursor) -> Bool {
  Count offset = 0;
  while (cursor.peek(S64(offset)).get_code().is_comment()) {
    offset++;
  }

  BAIL_IF(cursor.peek(S64(offset)).get_code() != Code::Type::Public);
  offset++;
  BAIL_IF(cursor.peek(S64(offset)).get_code() != Code::Type::Type);
  offset++;
  BAIL_IF(cursor.peek(S64(offset)).get_code() != Code::Type::Define);
  offset++;
  BAIL_IF(cursor.peek(S64(offset)).get_code() != Code::Type::Alias);
  offset++;
  BAIL_IF(cursor.peek(S64(offset)).get_code() != Code::Type::Assign);
  offset++;
  return cursor.peek(S64(offset)).get_code() == Code::Type::Source ||
         is_package_import(cursor, offset);
}

static auto parse_quoted(Cursor& cursor, View::Bytes expectation)
    -> Option<View::Bytes> {
  Token token = cursor.require(Code::Type::String, expectation);
  BAIL_IF(!token);
  View::Bytes text = token.caculate_text(cursor.get_source_text());
  if (!Lexicon::validate(Code::Type::String, text)) {
    cursor.create_token_error(token, "Import String is unterminated."_view);
    return {};
  }
  return text.slice(1, text.get_size() - 2);
}

static auto parse_source(Cursor& cursor) -> Option<View::Bytes> {
  BAIL_IF(!cursor.require(
      Code::Type::Source,
      "Source imports require `source(\"relative/path.ttx\")`."_view));
  BAIL_IF(!cursor.require(
      Code::Type::PackingStart,
      "Source imports require `(` before their local path."_view));
  auto path = parse_quoted(
      cursor, "Source imports require one quoted local path."_view);
  BAIL_IF(!path);
  BAIL_IF(!cursor.require(
      Code::Type::PackingEnd,
      "Source imports require `)` after their local path."_view));

  Bool invalid = path->is_empty() || (*path)[0] == '/' || (*path)[0] == '\\';
  for (Count index = 0; index < path->get_size(); index++) {
    invalid |= (*path)[index] == '\0';
  }
  if (invalid) {
    cursor.create_token_error(
        "Source import paths must be nonempty and relative."_view);
    return {};
  }
  return cursor.get_arena().proxy(*path);
}

static auto parse_named_string(Cursor& cursor, View::Bytes expected_name)
    -> Option<View::Bytes> {
  BAIL_IF(!cursor.require(
      Code::Type::AddressOp,
      "Package import arguments require a leading `.`."_view));
  Token name = cursor.require(
      Code::Type::Addressable,
      "Package import arguments require a named slot."_view);
  BAIL_IF(!name);
  if (name.caculate_text(cursor.get_source_text()) != expected_name) {
    cursor.create_token_error(
        name, "Package import argument is out of order."_view);
    return {};
  }
  BAIL_IF(!cursor.require(
      Code::Type::Assign, "Package import named arguments require `=`."_view));
  return parse_quoted(
      cursor, "Package import arguments require quoted String values."_view);
}

static auto parse_package(
    Cursor& cursor,
    View::Bytes& identity,
    Version& version) -> Bool {
  if (!is_package_import(cursor, 0)) {
    cursor.create_token_error(
        "Package imports require `package(.name = ..., .version = ...)`."_view);
    return False;
  }
  cursor.consume();
  BAIL_IF(!cursor.require(
      Code::Type::PackingStart,
      "Package imports require `(` before their arguments."_view));
  auto name = parse_named_string(cursor, "name"_view);
  BAIL_IF(!name);
  BAIL_IF(!cursor.require(
      Code::Type::PackingOp,
      "Package imports require both `name` and `version`."_view));
  auto version_text = parse_named_string(cursor, "version"_view);
  BAIL_IF(!version_text);
  BAIL_IF(!cursor.require(
      Code::Type::PackingEnd,
      "Package imports require `)` after their arguments."_view));

  version = Version::parse(*version_text);
  if (name->is_empty() || version.is_null()) {
    cursor.create_token_error(
        "Package imports require a nonempty name and canonical Major.Minor version."_view);
    return False;
  }
  identity = cursor.get_arena().proxy(*name);
  return True;
}

auto Parser::Import::parse(Cursor& cursor, const Documentation& documentation)
    -> Option<Language::Import::Description> {
  Token opening = cursor.require(
      Code::Type::Public, "Imports require `public` visibility."_view);
  BAIL_IF(!opening);
  Token name_token = cursor.require(
      Code::Type::Type,
      "Imports require one Type-shaped local Alias name."_view);
  BAIL_IF(!name_token);
  BAIL_IF(!cursor.require(
      Code::Type::Define, "Import names require `:` before `alias`."_view));
  BAIL_IF(!cursor.require(
      Code::Type::Alias, "Imports require the `alias` qualifier."_view));
  BAIL_IF(!cursor.require(
      Code::Type::Assign,
      "Import Aliases require `=` before their locator."_view));

  Language::Import::Kind kind = Language::Import::Kind::Source;
  View::Bytes locator;
  Version version;
  if (cursor.matches(Code::Type::Source)) {
    auto path = parse_source(cursor);
    BAIL_IF(!path);
    locator = *path;
  } else {
    kind = Language::Import::Kind::Package;
    BAIL_IF(!parse_package(cursor, locator, version));
  }

  Token closing = cursor.require(
      Code::Type::EndStatement,
      "Import Aliases require one terminating `;`."_view);
  BAIL_IF(!closing);
  View::Bytes name = name_token.caculate_text(cursor.get_source_text());
  return Language::Import::Description(
      name, documentation, Visibility::Public, kind, locator, version,
      Anchor::create(name_token, Span(opening, closing)));
}
