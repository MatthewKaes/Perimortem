// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/language/field.hpp"

#include "tetrodotoxin/library/language/initializer.hpp"
#include "tetrodotoxin/library/language/parser/expression.hpp"
#include "tetrodotoxin/library/language/types/composite.hpp"
#include "ttx/concept/invalid.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Perimortem::Utility;
using namespace Ttx::Concept;
using namespace Ttx::Lexical;
using namespace Ttx::Model;
using namespace Tetrodotoxin::Library;

static auto parse_policy(
    const Tetrodotoxin::Language::Definition& definition,
    Cursor& cursor,
    Language::Field::Exposure& exposure,
    Language::Field::Writability& writability) -> Bool {
  Count publication_count = 0;
  Count evaluation_count = 0;
  auto modifiers = definition.get_modifiers();
  for (Count i = 0; i < modifiers.get_size(); i++) {
    Token modifier = modifiers.get_data()[i];
    switch (modifier.get_code().get_type()) {
    case Code::Type::Public:
      exposure = Language::Field::Exposure::Public;
      publication_count++;
      break;
    case Code::Type::Private:
      exposure = Language::Field::Exposure::Private;
      publication_count++;
      break;
    case Code::Type::Expose:
      exposure = Language::Field::Exposure::Exposed;
      publication_count++;
      break;
    case Code::Type::State:
      writability = Language::Field::Writability::Internal;
      evaluation_count++;
      break;
    case Code::Type::Const:
      writability = Language::Field::Writability::Init;
      evaluation_count++;
      break;
    default:
      break;
    }
  }

  if (publication_count != 1) {
    cursor.create_token_error(
        definition.get_name_token(),
        "Library Fields require `public`, `private`, or `expose` "
        "publication."_view);
    return False;
  }
  if (evaluation_count > 1) {
    cursor.create_token_error(
        definition.get_name_token(),
        "Library Fields accept at most one evaluation modifier."_view);
    return False;
  }

  if (exposure == Language::Field::Exposure::Exposed &&
      writability != Language::Field::Writability::Internal) {
    cursor.create_token_error(
        "Library `expose` Fields require the `state` evaluation policy."_view);
    return False;
  }
  if (exposure == Language::Field::Exposure::Public &&
      writability == Language::Field::Writability::Internal) {
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
    Tetrodotoxin::Language::Definition& definition,
    const Type& host) -> Option<Field&> {
  auto transaction = cursor.branch();
  Exposure exposure = Exposure::Private;
  Writability writability = Writability::Full;
  Bool parsed_policy =
      parse_policy(definition, transaction, exposure, writability);
  BAIL_IF(!parsed_policy);

  if (definition.get_name_token().get_code() != Code::Type::Addressable) {
    transaction.create_token_error(
        definition.get_name_token(),
        "Library Fields require an addressable name."_view);
    return {};
  }

  Option<Access::Type> type;
  Option<Expression&> initializer;
  if (transaction.matches(Code::Type::Assign)) {
    transaction.consume();
    if (Initializer::is_next(transaction)) {
      transaction.create_token_error(
          "An inferred Library Field cannot use `new`."_view,
          "Name one exact Object Type before initialization begins."_view);
      return {};
    }

    initializer =
        Parser::Expression::parse(domain, materializations, transaction, host);
    BAIL_IF(!initializer);
  } else {
    auto authored_type = Access::Type::parse(transaction);
    BAIL_IF(!authored_type);
    type = *authored_type;

    if (transaction.matches(Code::Type::Assign)) {
      transaction.consume();
      if (Initializer::is_next(transaction)) {
        auto object_initializer =
            Initializer::parse(domain, materializations, transaction, host);
        BAIL_IF(!object_initializer);
        initializer = *object_initializer;
      } else {
        initializer = Parser::Expression::parse(
            domain, materializations, transaction, host);
      }
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

  Anchor anchor = Anchor::create(
      definition.get_name_token(),
      Span(definition.get_anchor().get_span().get_start(), terminator));
  Field& field = domain.construct_from<Field>([&]() -> Field {
    return Field(definition, type, anchor, initializer, host);
  });
  cursor.join(transaction);
  return field;
}

auto Language::Field::get_exposure() const -> Exposure {
  auto modifiers = definition.get_modifiers();
  for (Count i = 0; i < modifiers.get_size(); i++) {
    switch (modifiers.get_data()[i].get_code().get_type()) {
    case Code::Type::Public:
      return Exposure::Public;
    case Code::Type::Expose:
      return Exposure::Exposed;
    default:
      break;
    }
  }

  return Exposure::Private;
}

auto Language::Field::get_writability() const -> Writability {
  auto modifiers = definition.get_modifiers();
  for (Count i = 0; i < modifiers.get_size(); i++) {
    switch (modifiers.get_data()[i].get_code().get_type()) {
    case Code::Type::State:
      return Writability::Internal;
    case Code::Type::Const:
      return Writability::Init;
    default:
      break;
    }
  }

  return Writability::Full;
}

auto Language::Field::link_type(
    Tetrodotoxin::Language::Monograph& source,
    const Abstract& selected) -> Bool {
  if (!host.is<Language::Types::Composite>() || is_inferred()) {
    source.report(
        anchor,
        "Explicit Field linking requires one Composite host and Type route."_view,
        "Use the authored Field provenance with its exact completion path."_view);
    return False;
  }

  const Abstract& resolved =
      selected.is<Type>() ? selected : selected.resolve();
  auto selected_type = resolved.select<Type>();
  if (!selected_type) {
    source.report(
        get_type_anchor(),
        "Field Type route did not resolve to one stable Type."_view,
        "Publish the named Type in this Library context before linking."_view);
    return False;
  }

  if (type) {
    if (&type->get() == &*selected_type) {
      return True;
    }

    source.report(
        get_type_anchor(),
        "Field Type linking selected a different semantic identity."_view,
        "Repeat completion with the same resolved Type edge."_view);
    return False;
  }

  type = Reference<const Type>(*selected_type);
  return True;
}

auto Language::Field::link_initializer(
    Tetrodotoxin::Language::Monograph& monograph,
    Materializations& materializations) -> Bool {
  if (initializer_linked) {
    return True;
  }

  auto selected_initializer = initializer.visit(
      []() -> Option<Expression&> { return {}; },
      [](Expression& selected) -> Option<Expression&> { return selected; });
  if (!selected_initializer) {
    monograph.report(
        anchor, "Field initializer state is incomplete."_view,
        "Retain one initializer before linking restricted Field access."_view);
    return False;
  }

  // Composite retained this exact Field before linking and exposes only the
  // completed prefix inside its private transaction. That path authenticates
  // the real host without inventing another initializer context.
  Bool linked = selected_initializer->link(monograph, *this, materializations);
  BAIL_IF(!linked);

  if (!type) {
    const Abstract& resolved_type = selected_initializer->get_type().resolve();
    auto initializer_type = resolved_type.select<Type>();
    if (!initializer_type) {
      monograph.report(
          selected_initializer->get_anchor(),
          "Inferred Field initializer did not complete one stable Type."_view,
          "Use an initializer whose exact Type settles before Field "
          "publication."_view);
      return False;
    }

    type = Reference<const Type>(*initializer_type);
    initializer_linked = True;
    return True;
  }

  if (!selected_initializer->fits(type->get())) {
    monograph.report(
        selected_initializer->get_anchor(),
        "Field initializer does not fit the declared Field Type's semantic "
        "domain."_view,
        "Supply one value accepted by the declared Field Type."_view);
    return False;
  }

  initializer_linked = True;
  return True;
}

auto Language::Field::resolve() const -> const Abstract& {
  if (!type) {
    return Invalid::get_invalid();
  }

  return *this;
}

auto Language::Field::resolve_context(View::Bytes route) const
    -> const Abstract& {
  return host.visit<Language::Types::Composite>(
      [&](const Language::Types::Composite& composite) -> const Abstract& {
        return composite.resolve_context(route, *this);
      },
      [&](const Abstract&) -> const Abstract& {
        return host.resolve_context(route);
      });
}

auto Language::Field::get_initializer() const -> Option<const Expression&> {
  return initializer.visit(
      []() -> Option<const Expression&> { return {}; },
      [](const Expression& selected) -> Option<const Expression&> {
        return selected;
      });
}
