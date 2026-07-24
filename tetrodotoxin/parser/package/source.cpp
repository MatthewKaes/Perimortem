// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/parser/package/source.hpp"

#include "perimortem/memory/managed/bytes.hpp"
#include "perimortem/memory/managed/vector.hpp"

#include "tetrodotoxin/parser/comment.hpp"
#include "ttx/lexical/token.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Perimortem::System;
using namespace Perimortem::Utility;
using namespace Ttx::Concept;
using namespace Ttx::Lexical;
using namespace Tetrodotoxin;

using Resolution = Concept::Package::Resolution;

// String tokens include their quotes. Keep the returned view borrowed from the
// Source and reject an unterminated token before any declaration is published.
static auto extract_string(Cursor& cursor, Token token) -> Option<View::Bytes> {
  View::Bytes text = token.caculate_text(cursor.get_source_text());
  if (text.get_size() < 2 || text[0] != '"' ||
      text[text.get_size() - 1] != '"') {
    cursor.create_token_error(
        token,
        "Expected string constant is not terminated with a closing quote (\")."_view);
    return none;
  }

  return text.slice(1, text.get_size() - 2);
}

// Package names span several tokens, so collect the segments into one stable
// arena view before the Resolution leaves this parser stage.
static auto parse_package_name(Cursor& cursor) -> Option<View::Bytes> {
  Token segment = cursor.require(
      Code::Type::Type,
      "Expected a package name beginning with a Type-space name."_view);
  if (!segment) {
    return none;
  }

  Managed::Bytes package_name(
      cursor.get_arena(), segment.caculate_text(cursor.get_source_text()));
  while (cursor.matches(Code::Type::AddressOp)) {
    cursor.consume();

    segment = cursor.require(
        Code::Type::Type, "Expected a package name segment after `.`."_view);
    if (!segment) {
      return none;
    }

    package_name.append('.');
    package_name.concat(segment.caculate_text(cursor.get_source_text()));
  }

  return Option<View::Bytes>(cursor.get_arena().proxy(package_name));
}

// Member routes use one slash vocabulary. Dot segments disappear, safe parent
// segments collapse, and a parent that would leave the logical root rejects the
// declaration before Package construction observes a filesystem route.
static auto normalize_route(Cursor& cursor, View::Bytes authored)
    -> Option<View::Bytes> {
  if (authored.is_empty() || authored[0] == '/' ||
      authored[authored.get_size() - 1] == '/') {
    return none;
  }

  Managed::Bytes route(cursor.get_arena());
  Count segment_start = 0;
  for (Count i = 0; i <= authored.get_size(); i++) {
    if (i < authored.get_size() && authored[i] != '/') {
      if (authored[i] == '\\' || authored[i] == '\0') {
        return none;
      }
      continue;
    }

    View::Bytes segment = authored.slice(segment_start, i - segment_start);
    segment_start = i + 1;
    if (segment.is_empty()) {
      return none;
    }
    if (segment == "."_view) {
      continue;
    }
    if (segment == ".."_view) {
      if (route.get_size() == 0) {
        return none;
      }

      Count retained = 0;
      for (Count j = 0; j < route.get_size(); j++) {
        if (route[j] == '/') {
          retained = j;
        }
      }
      route.resize(retained);
      continue;
    }

    if (route.get_size() != 0) {
      route.append('/');
    }
    route.concat(segment);
  }

  if (route.get_size() == 0) {
    return none;
  }

  return Option<View::Bytes>(cursor.get_arena().proxy(route));
}

static auto has_resolution(
    View::Vector<Resolution> current,
    View::Bytes local_name) -> Bool {
  for (Count i = 0; i < current.get_size(); i++) {
    if (current[i].get_local_name() == local_name) {
      return True;
    }
  }

  return False;
}

static auto has_member(View::Vector<View::Bytes> current, View::Bytes route)
    -> Bool {
  for (Count i = 0; i < current.get_size(); i++) {
    if (current[i] == route) {
      return True;
    }
  }

  return False;
}

static auto parse_resolution(Cursor& cursor, View::Vector<Resolution> current)
    -> Option<Resolution> {
  // First close the local binding. Duplicate aliases are rejected before work
  // begins on an external Package identity.
  Token local_token = cursor.require(
      Code::Type::Type, "Expected a local name after `resolve`."_view);
  if (!local_token) {
    return none;
  }

  View::Bytes local_name = local_token.caculate_text(cursor.get_source_text());
  if (has_resolution(current, local_name)) {
    cursor.create_token_error(
        local_token, "Package resolution aliases must be unique."_view);
    return none;
  }

  if (!cursor.require(
          Code::Type::Define,
          "Expected `:` after the local resolution name."_view)) {
    return none;
  }

  // The external half is one exact Package name and Major.Minor Version. It is
  // never interpreted as a source Real value.
  Option<View::Bytes> package_name = parse_package_name(cursor);
  return package_name.visit(
      [](const None&) -> Option<Resolution> { return none; },
      [&](View::Bytes external_name) {
        if (!cursor.require(
                Code::Type::Assign,
                "Expected `=` before the exact package version."_view)) {
          return Option<Resolution>(none);
        }

        Token version_token = cursor.require(
            Code::Type::String,
            "Expected a quoted canonical Major.Minor package version."_view);
        if (!version_token) {
          return Option<Resolution>(none);
        }

        Option<View::Bytes> version_text =
            extract_string(cursor, version_token);
        return version_text.visit(
            [](const None&) -> Option<Resolution> { return none; },
            [&](View::Bytes text) {
              Version version = Version::parse(text);
              if (version.is_null()) {
                cursor.create_token_error(
                    version_token,
                    "Expected a canonical non-null package version such as `\"1.2\"`."_view);
                return Option<Resolution>(none);
              }

              if (!cursor.require(
                      Code::Type::EndStatement,
                      "Expected `;` after the package resolution."_view)) {
                return Option<Resolution>(none);
              }

              // Only a complete statement becomes visible to the outer
              // Package transaction.
              return Option<Resolution>(Resolution(
                  cursor.get_arena().proxy(local_name), external_name,
                  version));
            });
      });
}

static auto parse_member(Cursor& cursor, View::Vector<View::Bytes> current)
    -> Option<View::Bytes> {
  Token route_token = cursor.require(
      Code::Type::String, "Expected a quoted member Source route."_view);
  if (!route_token) {
    return none;
  }

  Option<View::Bytes> authored = extract_string(cursor, route_token);
  return authored.visit(
      [](const None&) -> Option<View::Bytes> { return none; },
      [&](View::Bytes source) {
        // Uniqueness applies to the normalized route. Two different authored
        // spellings must not select the same Source.
        Option<View::Bytes> normalized = normalize_route(cursor, source);
        return normalized.visit(
            [&](const None&) -> Option<View::Bytes> {
              cursor.create_token_error(
                  route_token,
                  "Member Source routes must stay beneath the package root."_view);
              return none;
            },
            [&](View::Bytes route) {
              if (has_member(current, route)) {
                cursor.create_token_error(
                    route_token, "Package member routes must be unique."_view);
                return Option<View::Bytes>(none);
              }

              if (!cursor.require(
                      Code::Type::EndStatement,
                      "Expected `;` after the member Source route."_view)) {
                return Option<View::Bytes>(none);
              }

              return Option<View::Bytes>(route);
            });
      });
}

auto Parser::Package::Source::parse(Cursor& cursor)
    -> Option<const Model::Package::Source&> {
  // The document comment and Dialect envelope establish that this is a Package
  // source before any container input is accepted.
  if (!cursor.matches(Code::Type::Comment)) {
    cursor.require(
        Code::Type::Comment,
        "Package files require an opening documentation comment."_view);
    return none;
  }

  const Documentation& documentation = Parser::Comment::parse(cursor);
  if (!cursor.require(
          Code::Type::Dialect,
          "Package files must begin with `dialect : Package;` after their documentation."_view)) {
    return none;
  }
  if (!cursor.require(
          Code::Type::Define,
          "Expected `:` after the package Dialect instruction."_view)) {
    return none;
  }

  Token dialect_token = cursor.require(
      Code::Type::Type, "Expected the Package Dialect name."_view);
  if (!dialect_token) {
    return none;
  }

  View::Bytes dialect = dialect_token.caculate_text(cursor.get_source_text());
  if (dialect != "Package"_view) {
    cursor.create_token_error(
        dialect_token,
        "A package descriptor must select the Package Dialect."_view);
    return none;
  }

  if (!cursor.require(
          Code::Type::EndStatement,
          "Expected `;` after the package Dialect instruction."_view)) {
    return none;
  }

  // Resolutions form the first ordered block because every member Source needs
  // the completed external bindings in its shared Environment.
  Managed::Vector<Resolution> resolutions(cursor.get_arena());
  while (cursor.matches(Code::Type::Resolve)) {
    cursor.consume();

    Option<Resolution> parsed = parse_resolution(cursor, resolutions);
    Bool added = parsed.visit(
        [](const None&) { return False; },
        [&](const Resolution& resolution) {
          resolutions.insert(resolution);
          return True;
        });
    if (!added) {
      return none;
    }
  }

  // Member declarations retain source order. Package construction later loads
  // and evaluates those routes in exactly this sequence.
  Managed::Vector<View::Bytes> members(cursor.get_arena());
  while (cursor.matches(Code::Type::Source)) {
    cursor.consume();

    Option<View::Bytes> parsed = parse_member(cursor, members);
    Bool added = parsed.visit(
        [](const None&) { return False; },
        [&](View::Bytes member) {
          members.insert(member);
          return True;
        });
    if (!added) {
      return none;
    }
  }

  if (cursor.matches(Code::Type::Resolve)) {
    cursor.create_token_error(
        "Package resolutions must precede member Source declarations."_view);
    return none;
  }

  // Publishing a token bookmark here would turn the result into suspended
  // parser state. Until Package evaluation exists, only a fully consumed
  // descriptor can leave this transaction.
  if (!cursor.matches(Code::Type::Terminal)) {
    cursor.create_token_error(
        "Package bodies require direct Package-Dialect semantic evaluation, which is not implemented."_view);
    return none;
  }

  const Model::Package::Source& source =
      cursor.get_arena().construct<Model::Package::Source>(
          documentation, resolutions, members);
  return source;
}
