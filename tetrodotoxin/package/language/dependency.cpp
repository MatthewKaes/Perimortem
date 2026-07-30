// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/package/language/dependency.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::System;
using namespace Perimortem::Utility;
using namespace Ttx::Lexical;
using namespace Tetrodotoxin;

// Qualified names retain one contiguous authored source span. Tokenization
// omits spacing, so adjacent token offsets prove that no internal whitespace
// was projected out of the retained name.
static auto parse_qualified_name(
    Cursor& cursor,
    Code::Type separator,
    View::Bytes initial_message,
    View::Bytes segment_message,
    View::Bytes whitespace_message) -> View::Bytes {
  Token first = cursor.require(Code::Type::Type, initial_message);
  if (!first) {
    return View::Bytes();
  }

  Token last = first;
  while (cursor.matches(separator)) {
    Token operator_token = cursor.current();
    const Count previous_end =
        Count(last.get_offset()) + Count(last.get_size());
    if (operator_token.get_offset() != previous_end) {
      cursor.create_expression_error(first, operator_token, whitespace_message);
      return View::Bytes();
    }

    cursor.consume();
    Token segment = cursor.require(Code::Type::Type, segment_message);
    if (!segment) {
      return View::Bytes();
    }

    const Count operator_end =
        Count(operator_token.get_offset()) + Count(operator_token.get_size());
    if (segment.get_offset() != operator_end) {
      cursor.create_expression_error(first, segment, whitespace_message);
      return View::Bytes();
    }

    last = segment;
  }

  const Count name_start = first.get_offset();
  const Count name_end = Count(last.get_offset()) + Count(last.get_size());
  return cursor.get_source_text().slice(name_start, name_end - name_start);
}

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
  if (text.get_size() < 2 || text[0] != '"' ||
      text[text.get_size() - 1] != '"') {
    cursor.create_token_error(
        token, "Package dependency version String is unterminated."_view);
    return {};
  }

  return text.slice(1, text.get_size() - 2);
}

auto Package::Language::Dependency::parse(Cursor& cursor)
    -> Option<Dependency> {
  if (!cursor.require(
          Code::Type::Resolve,
          "Expected a Package `resolve` statement."_view)) {
    cursor.recover_to_statement();
    return {};
  }

  View::Bytes local_name = parse_qualified_name(
      cursor, Code::Type::TypeAccessOp,
      "Expected an authored Type shaped semantic name."_view,
      "Semantic name qualification requires a Type segment after `::`."_view,
      "Semantic names cannot contain whitespace around `::`."_view);
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

  View::Bytes package_name = parse_qualified_name(
      cursor, Code::Type::AddressOp,
      "Expected an external Type shaped Package name."_view,
      "External Package qualification requires a Type segment after `.`."_view,
      "External Package names cannot contain whitespace around `.`."_view);
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

  if (!cursor.require(
          Code::Type::EndStatement,
          "Resolve statements require a terminating `;`."_view)) {
    cursor.recover_to_statement();
    return {};
  }

  return Dependency(local_name, package_name, version);
}
