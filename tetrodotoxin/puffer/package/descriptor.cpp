// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/puffer/package/descriptor.hpp"

#include "perimortem/memory/managed/bytes.hpp"
#include "perimortem/memory/managed/map.hpp"

#include "tetrodotoxin/interpreter/documentation.hpp"
#include "tetrodotoxin/model/dialect.hpp"
#include "ttx/concept/invalid.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Perimortem::System;
using namespace Tetrodotoxin::Puffer::Package;
using namespace Ttx::Concept;
using namespace Ttx::Lexical;

static auto read_type(Cursor& cursor, View::Bytes message, View::Bytes& result)
    -> Bool {
  Token token = cursor.require(Code::Type::Type, message);
  if (!token.is_valid()) {
    return False;
  }

  result = token.caculate_text(cursor.get_source_text());
  return True;
}

static auto read_string(
    Cursor& cursor,
    View::Bytes message,
    View::Bytes& result) -> Bool {
  Token token = cursor.require(Code::Type::String, message);
  if (!token.is_valid()) {
    return False;
  }

  View::Bytes encoded = token.caculate_text(cursor.get_source_text());
  if (encoded.get_size() < 2 || encoded[0] != '"' ||
      encoded[encoded.get_size() - 1] != '"') {
    cursor.create_token_error("Expected a terminated quoted string."_view);
    return False;
  }

  result = encoded.slice(1, encoded.get_size() - 2);
  return True;
}

static auto read_package_name(
    Allocator::Arena& arena,
    Cursor& cursor,
    View::Bytes& result) -> Bool {
  View::Bytes segment;
  if (!read_type(cursor, "Expected a package name."_view, segment)) {
    return False;
  }

  Managed::Bytes package_name(arena, segment);
  while (cursor.matches(Code::Type::AddressOp)) {
    cursor.consume();
    if (!read_type(
            cursor, "Expected a package-name segment after `.`."_view,
            segment)) {
      return False;
    }

    package_name.append('.');
    package_name.concat(segment);
  }

  result = arena.proxy(package_name.get_view());
  return True;
}

static auto valid_member_path(View::Bytes path) -> Bool {
  if (path.is_empty() || path[0] == '/' || path[path.get_size() - 1] == '/' ||
      path[path.get_size() - 1] == '\\') {
    return False;
  }

  Count segment_start = 0;
  for (Count i = 0; i <= path.get_size(); i++) {
    if (i < path.get_size() && path[i] != '/') {
      if (path[i] == '\\') {
        return False;
      }
      continue;
    }

    Count size = i - segment_start;
    if (size == 0 || (size == 1 && path[segment_start] == '.') ||
        (size == 2 && path[segment_start] == '.' &&
         path[segment_start + 1] == '.')) {
      return False;
    }
    segment_start = i + 1;
  }

  return True;
}

auto Descriptor::parse(
    Allocator::Arena& arena,
    const Tokenizer& tokenizer,
    const Abstract& dialects,
    Errors& errors) -> const Descriptor* {
  Cursor cursor(tokenizer, errors);
  const Documentation& documentation =
      Tetrodotoxin::Interpreter::Documentation::evaluate(cursor);

  Token dialect_instruction = cursor.require(
      Code::Type::Dialect,
      "Expected `dialect : Package;` at the start of package.ttx."_view);
  if (!dialect_instruction.is_valid() ||
      !cursor
           .require(
               Code::Type::Define,
               "Expected `:` after the package Dialect instruction."_view)
           .is_valid()) {
    return nullptr;
  }

  View::Bytes dialect;
  if (!read_type(cursor, "Expected the Package Dialect name."_view, dialect) ||
      dialect != "Package"_view) {
    if (!dialect.is_empty()) {
      cursor.create_token_error(
          "A package descriptor must select the Package Dialect."_view);
    }
    return nullptr;
  }
  const Abstract& selected_dialect = dialects.resolve_context(dialect);
  if (!selected_dialect.is<Model::Dialect>()) {
    cursor.create_token_error(
        "The package descriptor's Dialect is not installed."_view);
    return nullptr;
  }
  if (!cursor
           .require(
               Code::Type::EndStatement,
               "Expected `;` after the package Dialect instruction."_view)
           .is_valid()) {
    return nullptr;
  }

  Descriptor* descriptor_storage = arena.reserve<Descriptor>();
  Descriptor& descriptor = *new (descriptor_storage) Descriptor(
      arena, selected_dialect.assume<Model::Dialect>(), documentation,
      arena.proxy(tokenizer.get_source_text()),
      arena.proxy(tokenizer.get_source_path()));
  Managed::Map<View::Bytes, Bool> local_names(arena);

  while (cursor.matches(Code::Type::Resolve)) {
    cursor.consume();
    View::Bytes local_name;
    if (!read_type(
            cursor, "Expected a local name after `resolve`."_view,
            local_name) ||
        !cursor
             .require(
                 Code::Type::Define,
                 "Expected `:` after the local resolution name."_view)
             .is_valid()) {
      return nullptr;
    }
    if (local_names.find(local_name) != nullptr) {
      cursor.create_token_error(
          "Package binding names must be unique in one Environment."_view);
      return nullptr;
    }

    View::Bytes package_name;
    if (!read_package_name(arena, cursor, package_name) ||
        !cursor
             .require(
                 Code::Type::Assign,
                 "Expected `=` before the exact package version."_view)
             .is_valid()) {
      return nullptr;
    }

    View::Bytes version_text;
    if (!read_string(
            cursor,
            "Expected a quoted canonical Major.Minor package version."_view,
            version_text)) {
      return nullptr;
    }
    Version version = Version::parse(version_text);
    if (version.is_null()) {
      cursor.create_token_error(
          "Expected a canonical non-null package version such as `\"1.2\"`."_view);
      return nullptr;
    }
    if (!cursor
             .require(
                 Code::Type::EndStatement,
                 "Expected `;` after the package resolution."_view)
             .is_valid()) {
      return nullptr;
    }

    View::Bytes owned_name = arena.proxy(local_name);
    local_names.insert(owned_name, True);
    descriptor.resolutions.insert(
        Resolution(owned_name, package_name, version));
  }

  while (cursor.matches(Code::Type::Source)) {
    cursor.consume();
    View::Bytes local_name;
    if (!read_type(
            cursor, "Expected a local name after `source`."_view, local_name) ||
        !cursor
             .require(
                 Code::Type::Define,
                 "Expected `:` after the local Source name."_view)
             .is_valid()) {
      return nullptr;
    }
    if (local_names.find(local_name) != nullptr) {
      cursor.create_token_error(
          "Package binding names must be unique in one Environment."_view);
      return nullptr;
    }

    View::Bytes member_dialect;
    if (!read_type(
            cursor, "Expected the member Source Dialect."_view,
            member_dialect) ||
        !cursor
             .require(
                 Code::Type::Assign,
                 "Expected `=` before the member Source path."_view)
             .is_valid()) {
      return nullptr;
    }
    const Abstract& selected_member_dialect =
        dialects.resolve_context(member_dialect);
    if (!selected_member_dialect.is<Model::Dialect>()) {
      cursor.create_token_error(
          "The member Source Dialect is not installed."_view);
      return nullptr;
    }

    View::Bytes member_path;
    if (!read_string(
            cursor, "Expected a quoted member Source path."_view,
            member_path)) {
      return nullptr;
    }
    if (!valid_member_path(member_path)) {
      cursor.create_token_error(
          "Member Source paths must be normalized and relative."_view);
      return nullptr;
    }
    if (!cursor
             .require(
                 Code::Type::EndStatement,
                 "Expected `;` after the member Source."_view)
             .is_valid()) {
      return nullptr;
    }

    View::Bytes owned_name = arena.proxy(local_name);
    local_names.insert(owned_name, True);
    descriptor.members.insert(Member(
        owned_name, selected_member_dialect.assume<Model::Dialect>(),
        arena.proxy(member_path)));
  }

  if (cursor.matches(Code::Type::Resolve)) {
    cursor.create_token_error(
        "Package resolutions must precede member Source declarations."_view);
    return nullptr;
  }

  descriptor.body_token_index = cursor.get_token_index();
  return &descriptor;
}

auto Descriptor::evaluate(Model::Source& source, Errors& errors) const
    -> const Abstract& {
  if (source.get_text() != source_text || source.get_path() != source_path) {
    errors.create_general_error(
        "The package Descriptor does not belong to this Source."_view,
        "Parse and evaluate the same package.ttx source transaction."_view);
    return Invalid::get_invalid();
  }

  return source.evaluate(dialect.get(), errors, body_token_index);
}
