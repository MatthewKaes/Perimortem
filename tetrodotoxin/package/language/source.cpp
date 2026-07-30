// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/package/language/source.hpp"

#include "perimortem/system/path.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Perimortem::System;
using namespace Perimortem::Utility;
using namespace Ttx::Lexical;
using namespace Tetrodotoxin;

// A Source semantic name is one contiguous Type qualified span. Every operator
// and following segment must touch its neighbor in the authored bytes.
static auto parse_semantic_name(Cursor& cursor) -> View::Bytes {
  Token first = cursor.require(
      Code::Type::Type, "Expected an authored Type shaped semantic name."_view);
  if (!first) {
    return View::Bytes();
  }

  Token last = first;
  while (cursor.matches(Code::Type::TypeAccessOp)) {
    Token operator_token = cursor.current();
    const Count previous_end =
        Count(last.get_offset()) + Count(last.get_size());
    if (operator_token.get_offset() != previous_end) {
      cursor.create_expression_error(
          first, operator_token,
          "Semantic names cannot contain whitespace around `::`."_view);
      return View::Bytes();
    }

    cursor.consume();
    Token segment = cursor.require(
        Code::Type::Type,
        "Semantic name qualification requires a Type segment after `::`."_view);
    if (!segment) {
      return View::Bytes();
    }

    const Count operator_end =
        Count(operator_token.get_offset()) + Count(operator_token.get_size());
    if (segment.get_offset() != operator_end) {
      cursor.create_expression_error(
          first, segment,
          "Semantic names cannot contain whitespace around `::`."_view);
      return View::Bytes();
    }

    last = segment;
  }

  const Count name_start = first.get_offset();
  const Count name_end = Count(last.get_offset()) + Count(last.get_size());
  return cursor.get_source_text().slice(name_start, name_end - name_start);
}

// A closed String contributes only its inner path bytes. The Source parser
// keeps the Token so path diagnostics remain attached to the authored value.
static auto parse_quoted_path(Cursor& cursor, Token& token)
    -> Option<View::Bytes> {
  token = cursor.require(
      Code::Type::String,
      "Source paths must be closed quoted String values."_view);
  if (!token) {
    return {};
  }

  View::Bytes text = token.caculate_text(cursor.get_source_text());
  if (text.get_size() < 2 || text[0] != '"' ||
      text[text.get_size() - 1] != '"') {
    cursor.create_token_error(
        token, "Source path String is unterminated."_view);
    return {};
  }

  return text.slice(1, text.get_size() - 2);
}

auto Package::Language::Source::parse(Allocator::Arena& domain, Cursor& cursor)
    -> Option<Source> {
  if (!cursor.require(
          Code::Type::Source, "Expected a Package `source` statement."_view)) {
    cursor.recover_to_statement();
    return {};
  }

  View::Bytes local_name = parse_semantic_name(cursor);
  if (local_name.is_empty()) {
    cursor.recover_to_statement();
    return {};
  }

  Token relation = cursor.require(
      Code::Type::Addressable,
      "Source statements require exact `from` spelling."_view);
  if (!relation) {
    cursor.recover_to_statement();
    return {};
  }

  if (relation.caculate_text(cursor.get_source_text()) != "from"_view) {
    cursor.create_token_error(
        relation, "Source statements require exact `from` spelling."_view);
    cursor.recover_to_statement();
    return {};
  }

  Token path_token;
  auto payload = parse_quoted_path(cursor, path_token);
  if (!payload) {
    cursor.recover_to_statement();
    return {};
  }

  Path normalized_path(*payload);
  if (normalized_path.get_view().is_empty()) {
    cursor.create_token_error(
        path_token, "Source path cannot be empty or lexically invalid."_view);
    cursor.recover_to_statement();
    return {};
  }

  if (!cursor.require(
          Code::Type::EndStatement,
          "Source statements require a terminating `;`."_view)) {
    cursor.recover_to_statement();
    return {};
  }

  View::Bytes durable_path = domain.proxy(normalized_path.get_view());
  return Source(local_name, durable_path);
}
