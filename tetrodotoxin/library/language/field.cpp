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

static auto parse_policy(
    Cursor& cursor,
    Language::Field::Exposure& exposure,
    Language::Field::Writability& writability) -> Bool {
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
    return False;
  }

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
    return False;
  }
  if (exposure == Language::Field::Exposure::Public && state) {
    cursor.create_token_error(
        "Library state Fields require `private` or explicit `expose` "
        "publication."_view);
    return False;
  }

  return True;
}

auto Language::Field::interpret(
    Allocator::Arena& domain,
    Materializations& materializations,
    Cursor& cursor,
    const Documentation& documentation,
    const Abstract& source_context) -> Option<Source> {
  auto transaction = cursor.branch();
  Token opening = transaction.current();
  Exposure exposure = Exposure::Private;
  Writability writability = Writability::Full;
  Bool parsed_policy = parse_policy(transaction, exposure, writability);
  BAIL_IF(!parsed_policy);

  Token name_token = transaction.require(
      Code::Type::Addressable,
      "Library Fields require an addressable name."_view);
  BAIL_IF(!name_token);

  BAIL_IF(!transaction.require(
      Code::Type::Define,
      "Library Fields require `:` before their Type."_view));

  Option<Access::Type> type;
  Option<Expression&> initializer;
  if (transaction.matches(Code::Type::Assign)) {
    transaction.consume();
    if (transaction.matches(Code::Type::Addressable) &&
        transaction.current().caculate_text(transaction.get_source_text()) ==
            "new"_view) {
      transaction.create_token_error(
          "An inferred Library Field cannot use `new`."_view,
          "Name one exact Object Type before construction begins."_view);
      return {};
    }

    initializer = Parser::Expression::parse(
        domain, materializations, transaction, source_context);
    BAIL_IF(!initializer);
  } else {
    auto authored_type = Access::Type::parse(transaction);
    BAIL_IF(!authored_type);
    type = *authored_type;

    if (transaction.matches(Code::Type::Assign)) {
      transaction.consume();
      initializer = Parser::Expression::parse(
          domain, materializations, transaction, source_context);
      BAIL_IF(!initializer);
    } else if (writability != Writability::Full) {
      transaction.create_token_error(
          "Library state and const Fields require an initializer."_view);
      return {};
    }
  }

  Token terminator = transaction.require(
      Code::Type::EndStatement,
      "Library Fields require one terminating `;`."_view);
  BAIL_IF(!terminator);

  View::Bytes name = name_token.caculate_text(transaction.get_source_text());
  Source field(
      name, type, documentation, exposure, writability,
      Anchor::create(name_token, Span(opening, terminator)), initializer);
  cursor.join(transaction);
  return field;
}

auto Language::Field::link(
    Allocator::Arena& domain,
    Tetrodotoxin::Language::Monograph& source,
    const Type& host,
    Source& field,
    const Abstract& selected) -> Option<Field&> {
  if (!host.is<Language::Types::Structure>() || field.is_inferred()) {
    source.report(
        field.get_anchor(),
        "Explicit Field linking requires one Structure host and Type route."_view,
        "Use the authored Field provenance with its exact completion path."_view);
    return {};
  }

  const Abstract& resolved =
      selected.is<Type>() ? selected : selected.resolve();
  auto type = resolved.select<Type>();
  if (!type) {
    source.report(
        field.get_type_anchor(),
        "Field Type route did not resolve to one stable Type."_view,
        "Publish the named Type in this Library context before linking."_view);
    return {};
  }

  return domain.construct_from<Field>([&]() -> Field {
    return Field(field, Reference<const Type>(*type), host);
  });
}

auto Language::Field::link_inferred(
    Allocator::Arena& domain,
    Tetrodotoxin::Language::Monograph& source,
    Materializations& materializations,
    const Type& host,
    Source& field) -> Option<Field&> {
  if (!host.is<Language::Types::Structure>() || !field.is_inferred() ||
      !field.has_initializer()) {
    source.report(
        field.get_anchor(),
        "Inferred Field construction requires one Structure host and "
        "initializer."_view,
        "Retain the authored inference on its exact containing Type."_view);
    return {};
  }

  Field& inferred = domain.construct_from<Field>(
      [&]() -> Field { return Field(field, {}, host); });
  BAIL_IF(!inferred.link_initializer(source, materializations));

  return inferred;
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

  // Structure either published the complete Field set or admitted this exact
  // inferred candidate inside its private transaction. Both paths authenticate
  // the real host without inventing another initializer context.
  Bool linked = initializer->link(monograph, *this, materializations);
  BAIL_IF(!linked);

  if (!type) {
    const Abstract& resolved_type = initializer->get_type().resolve();
    auto initializer_type = resolved_type.select<Type>();
    if (!initializer_type) {
      monograph.report(
          initializer->get_anchor(),
          "Inferred Field initializer did not complete one stable Type."_view,
          "Use an initializer whose exact Type settles before Field "
          "publication."_view);
      return False;
    }

    type = Reference<const Type>(*initializer_type);
    initializer_linked = True;
    return True;
  }

  if (!initializer->fits(type->get())) {
    monograph.report(
        initializer->get_anchor(),
        "Field initializer does not fit the declared Field Type's semantic "
        "domain."_view,
        "Supply one value accepted by the declared Field Type."_view);
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
