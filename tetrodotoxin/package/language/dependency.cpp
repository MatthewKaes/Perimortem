// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/package/language/dependency.hpp"

#include "perimortem/utility/range.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::System;
using namespace Perimortem::Utility;
using namespace Ttx::Lexical;
using namespace Tetrodotoxin::Package;

auto parse_dependency(Cursor& cursor) -> View::Bytes {
  // Extract the first segment explicitly from the stream.
  auto token = cursor.require(
      Code::Type::Type, "Package imports must start with a `Type` name."_view);
  if (!token) {
    cursor.recover_to_statement();
  }

  // Extract all following segments by just extending a range until we hit a
  // pattern failure.
  Range segment_range(token.get_offset(), token.get_size());
  Token segment_start = token;
  while (cursor.get_code() == Code::Type::AddressOp) {
    token = cursor.require(
        Code::Type::Type,
        "Package import name segment doesn't follow `Type` name."_view);
    if (!token) {
      cursor.recover_to_statement();
      return {};
    }

    segment_range.extend(token.get_offset() + token.get_size());
  }

  // The dependency name can be extracted from the segment range but the
  // tokenizer strips whitespace so we need to validate that the range doesn't
  // contain whitespace.
  View::Bytes dependency_name =
      cursor.get_source_text().slice(segment_range.start, segment_range.size);
  for (Count i = 0; i < dependency_name.get_size(); i++) {
    switch (dependency_name[i]) {
    case ' ':
    case '\n':
    case '\t':
      cursor.create_expression_error(
          segment_start, cursor.current(),
          "Dependency name contains whitespace characters."_view);
      cursor.recover_to_statement();
      return {};
    default:
      break;
    }
  }

  return dependency_name;
}

auto parse_version(Cursor& cursor) -> Version {
  if (cursor.bail(
          Code::Type::Assign,
          "Package dependencies expect a provided pinned version."_view)) {
    return {};
  }

  // Extract the first segment explicitly from the stream.
  auto token = cursor.require(
      Code::Type::String,
      "Package target does not have required version."_view);
  if (!token) {
    cursor.recover_to_statement();
  }

  auto version = Version::parse(cursor.get_source_text());
  if (version.is_null()) {
    cursor.create_token_error(
        token, "Invalid version string provided as package version."_view);
    cursor.recover_to_statement();
  }

  return version;
}

// resolve [Type] : [Path] = "Version";
auto Language::Dependency::parse(Cursor& cursor) -> Option<Dependency> {
  // `resolve` just falls in the Addressable space so we'll need to do a proper
  // parse rather than a solo `require` as a keyword check.
  if (cursor.get_code() != Code::Type::Addressable ||
      cursor.get_text() != "resolve"_view) {
    return {};
  }

  // Extract the `Type` as the name to bind.
  cursor.consume();
  auto token = cursor.require(
      Code::Type::Type, "Expected a binding type name to resolve to."_view);
  if (!token) {
    cursor.recover_to_statement();
  }

  // Extract the name and check if there is a proper defines.
  View::Bytes import_type = token.caculate_text(cursor.get_text());
  if (cursor.bail(Code::Type::Define)) {
    return {};
  }

  auto dependency_name = parse_dependency(cursor);
  if (dependency_name.is_empty()) {
    return {};
  }

  if (cursor.bail(Code::Type::Assign)) {
    return {};
  }

  auto version = parse_version(cursor);
  if (version.is_null()) {
    return {};
  }

  // Require the end statement.
  // Use a bail here to consume any junk artirfacts.
  if (cursor.bail(Code::Type::EndStatement)) {
    return {};
  }

  return Dependency(import_type, dependency_name, version);
}
