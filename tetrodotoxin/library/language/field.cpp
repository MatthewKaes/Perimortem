// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/language/field.hpp"

#include "tetrodotoxin/library/language/parser/expression.hpp"
#include "tetrodotoxin/library/language/types/structure.hpp"

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

struct ParsedPolicy {
  Language::Field::Exposure exposure;
  Language::Field::Writability writability;
};

static auto parse_policy(Cursor& cursor) -> Option<ParsedPolicy> {
  Language::Field::Exposure exposure;
  if (cursor.matches(Code::Type::Public)) {
    cursor.consume();
    exposure = Language::Field::Exposure::Public;
  } else if (cursor.matches(Code::Type::Private)) {
    cursor.consume();
    exposure = Language::Field::Exposure::Private;
  } else if (cursor.matches(Code::Type::Expose)) {
    cursor.consume();
    exposure = Language::Field::Exposure::Exposed;
  } else {
    cursor.create_token_error(
        "Library Fields require `public`, `private`, or `expose` "
        "publication."_view);
    return {};
  }

  Language::Field::Writability writability = Language::Field::Writability::Full;
  Bool state = False;
  if (cursor.matches(Code::Type::State)) {
    cursor.consume();
    writability = Language::Field::Writability::Internal;
    state = True;
  } else if (cursor.matches(Code::Type::Const)) {
    cursor.consume();
    writability = Language::Field::Writability::Init;
  }

  if (exposure == Language::Field::Exposure::Exposed && !state) {
    cursor.create_token_error(
        "Library `expose` Fields require the `state` evaluation policy."_view);
    return {};
  }
  if (exposure == Language::Field::Exposure::Public && state) {
    cursor.create_token_error(
        "Library state Fields require `private` or explicit `expose` "
        "publication."_view);
    return {};
  }

  return ParsedPolicy{
    .exposure = exposure,
    .writability = writability,
  };
}

static auto parse_type(Cursor& cursor) -> Option<ParsedType> {
  Token first = cursor.require(
      Code::Type::Type, "Library Fields require a Type name."_view);
  if (!first) {
    return {};
  }

  // Field retains the complete spelling because the hosting Structure owns
  // route grammar. Only the selected exact Type enters the Addressable edge.
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

auto Language::Field::interpret(
    Allocator::Arena& domain,
    Materializations& materializations,
    Cursor& cursor,
    const Documentation& documentation,
    const Abstract& source_context) -> Option<Source> {
  auto transaction = cursor.branch();
  Token opening = transaction.current();
  auto policy = parse_policy(transaction);
  if (!policy) {
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

  Option<Expression&> initializer;
  if (transaction.matches(Code::Type::Assign)) {
    transaction.consume();
    initializer = Parser::Expression::parse(
        domain, materializations, transaction, source_context);
    if (!initializer) {
      return {};
    }
  } else if (policy->writability != Writability::Full) {
    transaction.create_token_error(
        "Library state and const Fields require an initializer."_view);
    return {};
  }

  Token terminator = transaction.require(
      Code::Type::EndStatement,
      "Library Fields require one terminating `;`."_view);
  if (!terminator) {
    return {};
  }

  View::Bytes name = name_token.caculate_text(transaction.get_source_text());
  Source field(
      name, type->route, documentation, policy->exposure, policy->writability,
      Anchor::create(name_token, Span(opening, terminator)), type->anchor,
      initializer);
  cursor.join(transaction);
  return field;
}

auto Language::Field::link(
    Allocator::Arena& domain,
    Tetrodotoxin::Language::Monograph& source,
    const Type& host,
    Source& field,
    const Abstract& selected) -> Option<Field&> {
  if (!host.is<Language::Types::Structure>()) {
    source.report(
        field.get_anchor(), "Field host is not one Library Structure."_view,
        "Construct the Field through its exact containing Type."_view);
    return {};
  }

  const Abstract& resolved =
      selected.is<Type>() ? selected : selected.resolve();
  auto type = resolved.visit<Type>(
      [](const Type& selected) -> Option<const Type&> { return selected; },
      [](const Abstract&) -> Option<const Type&> { return {}; });
  if (!type) {
    source.report(
        field.get_type_anchor(),
        "Field Type route did not resolve to one stable Type."_view,
        "Publish the named Type in this Library context before linking."_view);
    return {};
  }

  return domain.construct_from<Field>(
      [&]() -> Field { return Field(field, *type, host); });
}

auto Language::Field::link_initializer(
    Tetrodotoxin::Language::Monograph& monograph,
    Materializations& materializations) -> Bool {
  if (initializer_linked) {
    return True;
  }

  auto initializer = source.get_initializer();
  if (!initializer) {
    monograph.report(
        source.get_anchor(), "Field initializer state is incomplete."_view,
        "Retain one initializer before linking restricted Field access."_view);
    return False;
  }

  // Structure retains every Field before this barrier, so the exact Field can
  // authenticate its host without inventing another initializer context.
  Bool linked = initializer->link(monograph, *this, materializations);
  if (!linked) {
    return False;
  }

  initializer_linked = True;
  return True;
}

auto Language::Field::resolve_context(View::Bytes route) const
    -> const Abstract& {
  return host.visit<Language::Types::Structure>(
      [&](const Language::Types::Structure& structure) -> const Abstract& {
        return structure.resolve_context(route, *this);
      },
      [&](const Abstract&) -> const Abstract& {
        return host.resolve_context(route);
      });
}

auto Language::Field::Source::get_initializer() const
    -> Option<const Expression&> {
  return initializer.visit(
      []() -> Option<const Expression&> { return {}; },
      [](const Expression& selected) -> Option<const Expression&> {
        return selected;
      });
}

auto Language::Field::Source::get_initializer() -> Option<Expression&> {
  return initializer.visit(
      []() -> Option<Expression&> { return {}; },
      [](Expression& selected) -> Option<Expression&> { return selected; });
}

auto Language::Field::get_initializer() const -> Option<const Expression&> {
  return static_cast<const Source&>(source).get_initializer();
}
