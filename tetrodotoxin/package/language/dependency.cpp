// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/package/language/dependency.hpp"

#include "tetrodotoxin/package/language/parser/name.hpp"
#include "ttx/lexical/lexicon.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::System;
using namespace Perimortem::Utility;
using namespace Ttx::Lexical;
using namespace Tetrodotoxin;

// String Tokens include their authored delimiters. An unterminated token still
// has String Code, so both ends are checked before exposing the inner bytes.
static auto parse_quoted_version(Cursor& cursor, Token& token)
    -> Option<View::Bytes> {
  token = cursor.require(
      Code::Type::String, "Package dependency versions must be quoted."_view);
  if (!token) {
    return {};
  }

  View::Bytes text = token.caculate_text(cursor.get_source_text());
  if (!Lexicon::validate(Code::Type::String, text)) {
    cursor.create_token_error(
        token, "Package dependency version String is unterminated."_view);
    return {};
  }

  return text.slice(1, text.get_size() - 2);
}

auto Package::Language::Dependency::parse(Cursor& cursor, Span& span)
    -> Option<Dependency> {
  span = Span();

  Token resolve = cursor.require(
      Code::Type::Resolve, "Expected a Package `resolve` statement."_view);
  if (!resolve) {
    cursor.recover_to_statement();
    return {};
  }

  // Both names borrow their exact authored bytes. Their distinct grammar
  // owners preserve that identity while the span remains only coordinates.
  View::Bytes local_name = Parser::Name::parse_semantic(cursor);
  if (local_name.is_empty()) {
    cursor.recover_to_statement();
    return {};
  }

  if (!cursor.require(
          Code::Type::Define,
          "Resolve statements require `:` before the external Package name."_view)) {
    cursor.recover_to_statement();
    return {};
  }

  View::Bytes package_name = Parser::Name::parse_package(cursor);
  if (package_name.is_empty()) {
    cursor.recover_to_statement();
    return {};
  }

  if (!cursor.require(
          Code::Type::Assign,
          "Resolve statements require one `=` before the pinned version."_view)) {
    cursor.recover_to_statement();
    return {};
  }

  // Version validates its durable value before provenance can be published.
  // A quoted token alone is not a complete Dependency request.
  Token version_token;
  auto payload = parse_quoted_version(cursor, version_token);
  if (!payload) {
    cursor.recover_to_statement();
    return {};
  }

  Version version = Version::parse(*payload);
  if (version.is_null()) {
    cursor.create_token_error(
        version_token,
        "Package dependency version is not canonical `Major.Minor` text."_view);
    cursor.recover_to_statement();
    return {};
  }

  // The terminal is returned only after every durable field is complete.
  // Failed recovery therefore cannot manufacture a successful statement span.
  Token consumed_end_statement = cursor.require(
      Code::Type::EndStatement,
      "Resolve statements require a terminating `;`."_view);
  if (!consumed_end_statement) {
    cursor.recover_to_statement();
    return {};
  }

  span = Span(resolve, consumed_end_statement);
  return Dependency(local_name, package_name, version);
}
