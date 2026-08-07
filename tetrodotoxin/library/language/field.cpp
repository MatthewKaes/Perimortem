// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/language/field.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Perimortem::Utility;
using namespace Ttx::Concept;
using namespace Ttx::Lexical;
using namespace Ttx::Model;
using namespace Tetrodotoxin::Library;

struct ParsedType {
  View::Bytes route;
  Anchor anchor;
};

static auto parse_visibility(Cursor& cursor) -> Option<Language::Visibility> {
  if (cursor.matches(Code::Type::Public)) {
    cursor.consume();
    return Language::Visibility::Public;
  }
  if (cursor.matches(Code::Type::Private)) {
    cursor.consume();
    return Language::Visibility::Private;
  }

  cursor.create_token_error(
      "Library Fields require `public` or `private` visibility."_view);
  return {};
}

static auto parse_type(Cursor& cursor) -> Option<ParsedType> {
  Token first = cursor.require(
      Code::Type::Type, "Library Fields require a Type name."_view);
  if (!first) {
    return {};
  }

  // Field retains the complete authored spelling because its eventual context
  // owns route grammar. Only the linked Type identity enters the TTX edge.
  Token last = first;
  while (cursor.matches(Code::Type::TypeAccessOp)) {
    Token separator = cursor.current();
    Count previous_end = Count(last.get_offset()) + Count(last.get_size());
    if (separator.get_offset() != previous_end) {
      cursor.create_expression_error(
          Span(first, separator),
          "Qualified Field Types cannot contain whitespace around `::`."_view);
      return {};
    }

    cursor.consume();
    Token segment = cursor.require(
        Code::Type::Type,
        "Qualified Field Types require a Type after `::`."_view);
    if (!segment) {
      return {};
    }

    Count separator_end =
        Count(separator.get_offset()) + Count(separator.get_size());
    if (segment.get_offset() != separator_end) {
      cursor.create_expression_error(
          Span(first, segment),
          "Qualified Field Types cannot contain whitespace around `::`."_view);
      return {};
    }

    last = segment;
  }

  Count route_start = first.get_offset();
  Count route_end = Count(last.get_offset()) + Count(last.get_size());
  View::Bytes route =
      cursor.get_source_text().slice(route_start, route_end - route_start);
  return ParsedType{
    .route = route,
    .anchor = Anchor::create(first, Span(first, last)),
  };
}

class LinkedField : public Addressable {
 public:
  constexpr LinkedField(const Language::Field& field, const Type& type)
      : field(field), type(type) {}

  constexpr auto get_name() const -> View::Bytes override {
    return field.get_name();
  }

  constexpr auto get_documentation() const -> const Documentation& override {
    return field.get_documentation();
  }

  constexpr auto get_type() const -> const Type& override { return type; }

 private:
  const Language::Field& field;
  const Type& type;
};

auto Language::Field::interpret(
    Cursor& cursor,
    const Documentation& documentation) -> Option<Field> {
  auto transaction = cursor.branch();
  Token opening = transaction.current();
  auto visibility = parse_visibility(transaction);
  if (!visibility) {
    return {};
  }

  Token name_token = transaction.require(
      Code::Type::Addressable,
      "Library Fields require an addressable name."_view);
  if (!name_token) {
    return {};
  }
  if (!transaction.require(
          Code::Type::Define,
          "Library Fields require `:` before their Type."_view)) {
    return {};
  }

  auto type = parse_type(transaction);
  if (!type) {
    return {};
  }
  Token terminator = transaction.require(
      Code::Type::EndStatement,
      "Library Fields require one terminating `;`."_view);
  if (!terminator) {
    return {};
  }

  View::Bytes name = name_token.caculate_text(transaction.get_source_text());
  Field field(
      name, type->route, documentation, *visibility,
      Anchor::create(name_token, Span(opening, terminator)), type->anchor);
  cursor.join(transaction);
  return field;
}

auto Language::Field::link(
    Allocator::Arena& domain,
    Tetrodotoxin::Language::Monograph& source,
    const Abstract& context) -> Bool {
  const Abstract& selected = context.resolve_context(type_route);
  const Abstract& resolved =
      selected.is<Type>() ? selected : selected.resolve();
  auto type = resolved.visit<Type>(
      [](const Type& selected) -> Option<const Type&> { return selected; },
      [](const Abstract&) -> Option<const Type&> { return {}; });
  if (!type) {
    source.report(
        type_anchor,
        "Field Type route did not resolve to one stable Type."_view,
        "Publish the named Type in this Library context before linking."_view);
    return False;
  }

  if (stage == Stage::Linked) {
    if (&addressable->get().get_type() == &*type) {
      return True;
    }

    source.report(
        type_anchor, "Linked Field Type route changed semantic identity."_view,
        "Keep one exact Type identity for the complete graph lifetime."_view);
    return False;
  }

  // The authored owner reaches a TTX edge only after its exact Type settles.
  // LinkedField borrows Field for source facts, so the composite owner must
  // keep its complete Field inventory at a stable address before this call.
  const auto& linked = domain.construct<LinkedField>(*this, *type);
  addressable = Reference<const Addressable>(linked);
  stage = Stage::Linked;
  return True;
}

auto Language::Field::get_type() const -> Option<const Type&> {
  return addressable.visit(
      []() -> Option<const Type&> { return {}; },
      [](const Reference<const Addressable>& selected) -> Option<const Type&> {
        return selected.get().get_type();
      });
}

auto Language::Field::get_addressable() const -> Option<const Addressable&> {
  return addressable.visit(
      []() -> Option<const Addressable&> { return {}; },
      [](const Reference<const Addressable>& selected)
          -> Option<const Addressable&> { return selected.get(); });
}
