// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/language/parser/layout.hpp"

#include "perimortem/memory/managed/vector.hpp"

#include "ttx/concept/reference.hpp"
#include "ttx/lexical/span.hpp"
#include "ttx/model/alias.hpp"
#include "ttx/model/layouts/fluid.hpp"
#include "ttx/model/layouts/named.hpp"
#include "ttx/model/type.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Perimortem::Utility;
using namespace Ttx::Concept;
using namespace Ttx::Lexical;
using namespace Ttx::Model;
using namespace Tetrodotoxin::Library::Language;

static auto parse_type(Cursor& cursor, const Abstract& context)
    -> Option<const Type&> {
  Token first = cursor.require(
      Code::Type::Type, "Library Layouts require a Type name."_view);
  if (!first) {
    return {};
  }

  // Qualification remains one borrowed authored route because the supplied
  // Abstract owns its route grammar. Splitting it here would manufacture a
  // second lookup policy and lose the exact query that selected the Type.
  Token last = first;
  while (cursor.matches(Code::Type::TypeAccessOp)) {
    Token separator = cursor.current();
    Count previous_end = Count(last.get_offset()) + Count(last.get_size());
    if (separator.get_offset() != previous_end) {
      cursor.create_expression_error(
          Span(first, separator),
          "Qualified Type routes cannot contain whitespace around `::`."_view);
      return {};
    }

    cursor.consume();
    Token segment = cursor.require(
        Code::Type::Type,
        "Qualified Type routes require a Type segment after `::`."_view);
    if (!segment) {
      return {};
    }

    Count separator_end =
        Count(separator.get_offset()) + Count(separator.get_size());
    if (segment.get_offset() != separator_end) {
      cursor.create_expression_error(
          Span(first, segment),
          "Qualified Type routes cannot contain whitespace around `::`."_view);
      return {};
    }

    last = segment;
  }

  Count route_start = first.get_offset();
  Count route_end = Count(last.get_offset()) + Count(last.get_size());
  View::Bytes route =
      cursor.get_source_text().slice(route_start, route_end - route_start);
  const Abstract& resolved = context.resolve_context(route).resolve();
  if (!resolved.is<Type>()) {
    cursor.create_expression_error(
        Span(first, last),
        "Library Layout Type route did not resolve to a complete Type."_view);
    return {};
  }

  return static_cast<const Type&>(resolved);
}

static auto contains_name(
    View::Vector<View::Bytes> names,
    View::Bytes candidate) -> Bool {
  for (Count i = 0; i < names.get_size(); i++) {
    if (names[i] == candidate) {
      return True;
    }
  }

  return False;
}

static auto parse_bracketed(
    Allocator::Arena& domain,
    Cursor& cursor,
    const Abstract& context) -> Option<const Layout&> {
  cursor.consume();
  if (cursor.matches(Code::Type::LayoutEnd)) {
    cursor.consume();
    return domain.construct<Layouts::Fluid>();
  }

  const Bool named = cursor.matches(Code::Type::AddressOp);
  if (!named && !cursor.matches(Code::Type::Type)) {
    cursor.create_token_error(
        "Library Layout entries must be Types or named `.value : Type` "
        "edges."_view);
    return {};
  }

  Managed::Vector<Reference<Abstract>> entries(domain);
  Managed::Vector<View::Bytes> names(domain);

  // The first entry fixes the complete Layout mode. Keeping that decision at
  // the opening boundary rejects ambiguous fitting before any Layout is
  // published, even though failed Arena allocations can remain unreachable.
  while (!cursor.matches(Code::Type::LayoutEnd)) {
    if (named) {
      if (!cursor.matches(Code::Type::AddressOp)) {
        cursor.create_token_error(
            "Named and unnamed entries cannot share one Library Layout."_view);
        return {};
      }

      cursor.consume();
      Token name_token = cursor.require(
          Code::Type::Addressable,
          "Named Library Layout entries require a name after `.`."_view);
      if (!name_token) {
        return {};
      }

      View::Bytes name = name_token.caculate_text(cursor.get_source_text());
      if (contains_name(names.get_view(), name)) {
        cursor.create_token_error(
            name_token, "Duplicate name in one Library Layout."_view);
        return {};
      }

      if (!cursor.require(
              Code::Type::Define,
              "Named Library Layout entries require `:` before the Type."_view)) {
        return {};
      }

      Option<const Type&> type = parse_type(cursor, context);
      if (!type) {
        return {};
      }

      // Alias keeps the authored parameter name on a real graph identity while
      // resolution still reaches the exact Type supplied by the semantic
      // context. Named therefore needs no copied parameter record.
      const Alias& edge = domain.construct<Alias>(name, *type);
      names.insert(name);
      entries.insert(edge);
    } else {
      if (cursor.matches(Code::Type::AddressOp)) {
        cursor.create_token_error(
            "Named and unnamed entries cannot share one Library Layout."_view);
        return {};
      }

      Option<const Type&> type = parse_type(cursor, context);
      if (!type) {
        return {};
      }

      entries.insert(*type);
    }

    if (cursor.matches(Code::Type::LayoutEnd)) {
      break;
    }

    if (!cursor.require(
            Code::Type::PackingOp,
            "Library Layout entries require `,` or the closing `]`."_view)) {
      return {};
    }

    if (cursor.matches(Code::Type::LayoutEnd)) {
      break;
    }
  }

  if (!cursor.require(
          Code::Type::LayoutEnd,
          "Library Layouts require a closing `]`."_view)) {
    return {};
  }

  if (named) {
    return domain.construct<Layouts::Named>(entries.get_view());
  }

  return domain.construct<Layouts::Fluid>(entries.get_view());
}

auto Parser::Layout::parse(
    Allocator::Arena& domain,
    Cursor& cursor,
    const Abstract& context) -> Option<const Ttx::Concept::Layout&> {
  if (cursor.matches(Code::Type::LayoutStart)) {
    return parse_bracketed(domain, cursor, context);
  }

  Option<const Type&> type = parse_type(cursor, context);
  if (!type) {
    return {};
  }

  Managed::Vector<Reference<Abstract>> entry(domain);
  entry.insert(*type);
  return domain.construct<Layouts::Fluid>(entry.get_view());
}
