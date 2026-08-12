// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/language/field.hpp"

#include "tetrodotoxin/library/language/initializer.hpp"
#include "tetrodotoxin/library/language/model/parser/pack.hpp"
#include "tetrodotoxin/library/language/monograph.hpp"
#include "tetrodotoxin/library/language/types/composite.hpp"
#include "ttx/concept/invalid.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Perimortem::Utility;
using namespace Ttx::Concept;
using namespace Ttx::Lexical;
using namespace Ttx::Model;
using namespace Tetrodotoxin::Library;

static auto parse_writability(
    const Tetrodotoxin::Language::Definition& definition,
    Cursor& cursor) -> Option<Language::Writability> {
  auto modifiers = definition.get_modifiers();
  if (modifiers.get_size() > 1) {
    cursor.create_token_error(
        modifiers.get_data()[1],
        "Library Fields accept at most one evaluation modifier."_view);
    return {};
  }

  Language::Writability writability = Language::Writability::Full;
  if (!modifiers.is_empty()) {
    switch (modifiers.get_data()[0].get_code().get_type()) {
    case Code::Type::State:
      writability = Language::Writability::Internal;
      break;
    case Code::Type::Const:
      writability = Language::Writability::Init;
      break;
    default:
      cursor.create_token_error(
          modifiers.get_data()[0],
          "Library Fields accept only `state` or `const` evaluation."_view);
      return {};
    }
  }

  Tetrodotoxin::Language::Visibility visibility = definition.get_visibility();
  if (visibility == Tetrodotoxin::Language::Visibility::Exposed &&
      writability != Language::Writability::Internal) {
    cursor.create_token_error(
        definition.get_visibility_token(),
        "Library `expose` Fields require the `state` evaluation policy."_view);
    return {};
  }
  if (visibility == Tetrodotoxin::Language::Visibility::Public &&
      writability == Language::Writability::Internal) {
    cursor.create_token_error(
        modifiers.get_data()[0],
        "Library state Fields require `private` or explicit `expose` "
        "publication."_view);
    return {};
  }

  return writability;
}

auto Language::Field::interpret(
    Allocator::Arena& domain,
    Monograph& source,
    Cursor& cursor,
    Tetrodotoxin::Language::Definition& definition) -> Option<Field&> {
  auto transaction = cursor.branch();
  auto host = definition.get_host().select<Language::Types::Composite>();
  BAIL_IF(!host);
  auto writability = parse_writability(definition, transaction);
  BAIL_IF(!writability);

  if (definition.get_name_token().get_code() != Code::Type::Addressable) {
    transaction.create_token_error(
        definition.get_name_token(),
        "Library Fields require an addressable name."_view);
    return {};
  }

  Option<TypeReference> type;
  Option<Model::Pack&> initializer;
  if (transaction.matches(Code::Type::Assign)) {
    transaction.consume();
    if (Initializer::is_next(transaction)) {
      transaction.create_token_error(
          "An inferred Library Field cannot use `new`."_view,
          "Name one exact Object Type before initialization begins."_view);
      return {};
    }

    initializer = Model::Parser::Pack::parse(domain, source, transaction);
    BAIL_IF(!initializer);
  } else {
    auto authored_type = TypeReference::parse(source, transaction);
    BAIL_IF(!authored_type);
    type = *authored_type;

    if (transaction.matches(Code::Type::Assign)) {
      transaction.consume();
      if (Initializer::is_next(transaction)) {
        auto object_initializer =
            Initializer::parse(domain, source, transaction);
        BAIL_IF(!object_initializer);
        initializer = *object_initializer;
      } else {
        initializer = Model::Parser::Pack::parse(domain, source, transaction);
      }
      BAIL_IF(!initializer);
    } else if (*writability != Writability::Full) {
      transaction.create_token_error(
          "Library state and const Fields require an initializer."_view);
      return {};
    }
  }

  Token terminator = transaction.require(
      Code::Type::EndStatement,
      "Library Fields require one terminating `;`."_view);
  BAIL_IF(!terminator);

  BAIL_IF(!definition.complete(definition.get_name_token(), terminator));
  Field& field = domain.construct_from<Field>([&]() -> Field {
    return Field(definition, *writability, type, initializer);
  });
  cursor.join(transaction);
  return field;
}

auto Language::Field::link_type(Tetrodotoxin::Language::Monograph& source)
    -> Bool {
  auto composite = get_host().select<Language::Types::Composite>();
  if (!composite || !type_reference) {
    source.report(
        get_anchor(),
        "Explicit Field linking requires one Composite host and Type route."_view,
        "Use the authored Field provenance with its exact completion path."_view);
    return False;
  }

  const Abstract& selected = composite->resolve_type(*type_reference);
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
  if (selected_type->get_layout().is_empty()) {
    source.report(
        get_type_anchor(), "Field cannot bind an empty Type Layout."_view,
        "Use the empty Type as a Static namespace or choose a Type with one "
        "value leaf."_view);
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
    Tetrodotoxin::Language::Monograph& monograph) -> Bool {
  if (initializer_linked) {
    return True;
  }

  auto selected_initializer = initializer.visit(
      []() -> Option<Model::Pack&> { return {}; },
      [](Model::Pack& selected) -> Option<Model::Pack&> { return selected; });
  if (!selected_initializer) {
    monograph.report(
        get_anchor(), "Field initializer state is incomplete."_view,
        "Retain one initializer before linking restricted Field access."_view);
    return False;
  }

  // Composite retained this exact Field before linking and exposes only the
  // completed prefix inside its private transaction. That path authenticates
  // the real host without inventing another initializer context.
  // The Field remains the lexical owner of bare names. Its host Type travels
  // separately as access authority so nested expressions never have to infer
  // scope from the concrete declaration category.
  BAIL_IF(!selected_initializer->link(monograph, *this, get_host()));

  if (!type) {
    if (selected_initializer->get_layout().get_size() != 1) {
      monograph.report(
          get_anchor(),
          "Inferred Field initializer must produce exactly one value."_view,
          "Name an explicit receiving Type for empty or multi-value flow."_view);
      return False;
    }

    const Abstract& result_type = selected_initializer->get_type();
    // A scalar Pack can expose one exact Composite Type before that Type
    // finishes its instance Layout. Inference retains that real identity
    // directly rather than resolving it to the incomplete sentinel.
    const Abstract& resolved_type =
        result_type.is<Type>() ? result_type : result_type.resolve();
    auto initializer_type = resolved_type.select<Type>();
    if (!initializer_type) {
      monograph.report(
          get_anchor(),
          "Inferred Field initializer did not complete one stable Type."_view,
          "Use an initializer whose exact Type settles before Field "
          "publication."_view);
      return False;
    }
    if (initializer_type->get_layout().is_empty()) {
      monograph.report(
          get_anchor(), "Inferred Field cannot bind an empty Type Layout."_view,
          "Keep the empty result as flow or infer from a Type with one value "
          "leaf."_view);
      return False;
    }

    type = Reference<const Type>(*initializer_type);
    initializer_linked = True;
    return True;
  }

  if (!selected_initializer->fits(type->get())) {
    monograph.report(
        get_anchor(),
        "Field initializer Pack does not fit the declared Field Type's "
        "Layout."_view,
        "Supply the complete value flow accepted by the declared Field "
        "Type."_view);
    return False;
  }

  initializer_linked = True;
  return True;
}

auto Language::Field::validate_publication(
    Tetrodotoxin::Language::Monograph& source) const -> Bool {
  if (!get_definition().is_published()) {
    return True;
  }

  auto composite = get_host().select<Language::Types::Composite>();
  if (!composite) {
    source.report(
        get_anchor(), "A published Field has no Composite host."_view,
        "Retain the Field on the Composite that owns its declaration."_view);
    return False;
  }

  Bool reachable = type_reference.visit(
      [&]() { return composite->is_externally_reachable(get_type()); },
      [&](const TypeReference& reference) {
        return Bool(
            &composite->resolve_exported_type(reference) == &get_type());
      });
  if (reachable) {
    return True;
  }

  source.report(
      get_type_anchor().visit(
          [&]() -> Option<Anchor> { return get_anchor(); },
          [](Anchor selected) -> Option<Anchor> { return selected; }),
      "Externally readable Field publishes an unreachable Type."_view,
      "Keep the Field private or publish its exact Type."_view);
  return False;
}

auto Language::Field::finalize() -> void {
  initializer.visit(
      []() {}, [](Model::Pack& selected) { selected.finalize(); });
}

auto Language::Field::resolve() const -> const Abstract& {
  if (!type) {
    return Invalid::get_invalid();
  }

  return *this;
}

auto Language::Field::resolve_context(View::Bytes route) const
    -> const Abstract& {
  const Type& host = get_host();
  return host.visit<Language::Types::Composite>(
      [&](const Language::Types::Composite& composite) -> const Abstract& {
        // Field initializers may name another local Addressable without an
        // explicit receiver. This is lexical declaration lookup, distinct from
        // `.` selection over a value's Layout.
        const Abstract& addressable =
            composite.resolve_lexical_addressable(route, host);
        if (&addressable != &Invalid::get_invalid()) {
          return addressable;
        }

        return composite.resolve_type_root(route, host);
      },
      [&](const Abstract&) -> const Abstract& {
        return host.resolve_context(route);
      });
}

auto Language::Field::get_initializer() const -> Option<const Model::Pack&> {
  return initializer.visit(
      []() -> Option<const Model::Pack&> { return {}; },
      [](const Model::Pack& selected) -> Option<const Model::Pack&> {
        return selected;
      });
}
