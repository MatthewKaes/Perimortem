// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/library/language/field.hpp"

#include "tetrodotoxin/library/language/diagnostics.hpp"
#include "tetrodotoxin/library/language/expression.hpp"
#include "tetrodotoxin/library/language/fold.hpp"
#include "ttx/bootstrap/concept/constant.hpp"
#include "ttx/bootstrap/concept/none.hpp"
#include "ttx/bootstrap/concept/unknown.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Perimortem::Utility;
using namespace Ttx::Concept;
using namespace Ttx::Lexical;
using namespace Ttx::Model;
using namespace Tetrodotoxin::Library;

auto Language::Field::create_authored(
    Allocator::Arena& domain,
    Tetrodotoxin::Language::Definition& definition,
    Writability writability,
    Option<TypeReference> type_reference,
    Option<Model::Pack&> initializer) -> Field& {
  return create(domain, definition, writability, type_reference, initializer);
}

auto Language::Field::create(
    Allocator::Arena& domain,
    Tetrodotoxin::Language::Definition& definition,
    Writability writability,
    Option<TypeReference> type_reference,
    Option<Model::Pack&> initializer) -> Field& {
  return domain.construct_from<Field>([&]() -> Field {
    return Field(domain, definition, writability, type_reference, initializer);
  });
}

auto Language::Field::create_generated(
    Allocator::Arena& domain,
    Tetrodotoxin::Language::Definition& definition,
    Writability writability,
    const Model::Type& type) -> Field& {
  return domain.construct_from<Field>([&]() -> Field {
    return Field(domain, definition, writability, {}, {}, &type, True);
  });
}

auto Language::Field::create_generated(
    Allocator::Arena& domain,
    Tetrodotoxin::Language::Definition& definition,
    Writability writability,
    TypeReference type_reference) -> Field& {
  return domain.construct_from<Field>([&]() -> Field {
    return Field(domain, definition, writability, type_reference, {}, {}, True);
  });
}

auto Language::Field::retain_generated_type(const Model::Type& selected)
    -> Bool {
  BAIL_IF(type_reference || selected.get_layout().is_empty());
  generated = True;
  if (type) {
    return *type == &selected;
  }
  type = &selected;
  return True;
}

auto Language::Field::retain_generated_initializer(const Field& requirement)
    -> Bool {
  BAIL_IF(!generated || initializer || !initializer_linked);
  auto selected = requirement.get_initializer();
  if (!selected) {
    return True;
  }

  initializer = const_cast<Model::Pack&>(*selected);
  if (writability == Writability::Constant) {
    const Abstract& answer = resolve_fold();
    return &answer != &None::get_none() &&
           Bool(Ttx::Concept::Constant::prove(answer));
  }
  return True;
}

auto Language::Field::get_type() const -> const Abstract& {
  if (type) {
    return **type;
  }
  if (!type_reference) {
    return Unknown::get_unknown();
  }

  Option<const Abstract&> selected;
  type_reference->resolve_lexical(*this).visit(
      [&](const Abstract& answer) { selected = answer; },
      [](const TypeReference::Failure&) {});
  auto selected_type = selected ? selected->select<Language::Model::Type>()
                                : Option<const Language::Model::Type&>();
  return selected_type ? static_cast<const Abstract&>(*selected_type)
                       : static_cast<const Abstract&>(Unknown::get_unknown());
}

auto Language::Field::link_declaration_type(Cursor& cursor) -> Bool {
  if (!type_reference) {
    return True;
  }

  auto selected = type_reference->resolve_authored(cursor, *this);
  BAIL_IF(!selected);
  auto selected_type = selected->select<Language::Model::Type>();
  if (!selected_type) {
    cursor.create_expression_error(
        get_type_anchor(),
        "Field Type route did not resolve to one stable Type."_view,
        "Publish the named Type in this Library context before linking."_view);
    return False;
  }

  if (selected_type->get_layout().is_empty()) {
    cursor.create_expression_error(
        get_type_anchor(), "Field cannot bind an empty Type Layout."_view,
        "Use the empty Type as a Static namespace or choose a Type with one "
        "value leaf."_view);
    return False;
  }

  if (type) {
    if (*type == &*selected_type) {
      return True;
    }

    cursor.create_expression_error(
        get_type_anchor(),
        "Field Type linking selected a different semantic identity."_view,
        "Repeat completion with the same resolved Type edge."_view);
    return False;
  }

  type = &*selected_type;
  return True;
}

auto Language::Field::link_restored_declaration_type() -> Bool {
  if (!type_reference) {
    return True;
  }

  Option<const Model::Type&> selected_type;
  type_reference->resolve_lexical(*this).visit(
      [&](const Abstract& selected) {
        selected_type = selected.select<Model::Type>();
      },
      [](const TypeReference::Failure&) {});
  BAIL_IF(!selected_type || selected_type->get_layout().is_empty());
  type = &*selected_type;
  return True;
}

auto Language::Field::link_restored_declaration_initializer() -> Bool {
  if (initializer_linked) {
    return True;
  }

  auto selected_initializer = initializer.visit(
      []() -> Option<Model::Pack&> { return {}; },
      [](Model::Pack& selected) -> Option<Model::Pack&> { return selected; });
  BAIL_IF(!selected_initializer);
  BAIL_IF(!selected_initializer->link_restored(*this, get_host()));
  BAIL_IF(!selected_initializer->is_complete());

  if (!type) {
    BAIL_IF(selected_initializer->get_layout().get_size() != 1);
    const Abstract& output = selected_initializer->get_type();
    const Abstract& resolved =
        output.is<Model::Type>() ? output : output.resolve();
    auto inferred = resolved.select<Model::Type>();
    BAIL_IF(!inferred || inferred->get_layout().is_empty());
    type = &*inferred;
  } else {
    BAIL_IF(!selected_initializer->fits_into(**type));
  }

  initializer_linked = True;
  const Abstract& answer = resolve_fold();
  return writability != Writability::Constant ||
         (&answer != &None::get_none() &&
          Bool(Ttx::Concept::Constant::prove(answer)));
}

auto Language::Field::link_inferred_declaration_type(Cursor& cursor) -> Bool {
  if (type_reference) {
    return True;
  }

  return link_declaration_initializer(cursor);
}

auto Language::Field::link_declaration_initializer(Cursor& cursor) -> Bool {
  if (initializer_linked) {
    return True;
  }

  auto selected_initializer = initializer.visit(
      []() -> Option<Model::Pack&> { return {}; },
      [](Model::Pack& selected) -> Option<Model::Pack&> { return selected; });
  if (!selected_initializer) {
    cursor.create_expression_error(
        get_anchor(), "Field initializer state is incomplete."_view,
        "Retain one initializer before linking restricted Field access."_view);
    return False;
  }

  // The host retained this exact Field before linking and exposes only its
  // completed declaration prefix. That path authenticates the real host
  // without inventing another initializer context.
  // The Field remains the lexical owner of bare names. Its host Type travels
  // separately as access authority so nested expressions never have to infer
  // scope from the concrete declaration category.
  BAIL_IF(!selected_initializer->link(cursor, *this, get_host()));
  // Linking a Type name is valid when a later access consumes its identity.
  // Field is a value owner, so it proves real Pack flow before reading Layout.
  if (!selected_initializer->is_complete()) {
    cursor.create_expression_error(
        get_anchor(), "Field initializer did not produce value flow."_view,
        "Use a Type result only as an access receiver."_view);
    return False;
  }

  if (!type) {
    if (selected_initializer->get_layout().get_size() != 1) {
      auto report = cursor.create_report(get_anchor());
      report << "Cannot infer Field '"_view << get_name() << "' from "_view
             << selected_initializer->get_layout().get_size()
             << " initializer values.\nSource produces: "_view;
      Language::Diagnostics::write_pack(report, *selected_initializer);
      report.get_hint()
          << "Provide exactly one value or declare the Field's receiving Type."_view;
      return False;
    }

    const Abstract& result_type = selected_initializer->get_type();
    // A scalar Pack can expose one exact Type before that Type finishes its
    // Layout. Inference retains that real identity directly rather than
    // resolving it to the incomplete sentinel.
    const Abstract& resolved_type = result_type.is<Language::Model::Type>()
                                        ? result_type
                                        : result_type.resolve();
    auto initializer_type = resolved_type.select<Language::Model::Type>();
    if (!initializer_type) {
      cursor.create_expression_error(
          get_anchor(),
          "Inferred Field initializer did not complete one stable Type."_view,
          "Use an initializer whose exact Type settles before Field "
          "publication."_view);
      return False;
    }

    if (initializer_type->get_layout().is_empty()) {
      cursor.create_expression_error(
          get_anchor(), "Inferred Field cannot bind an empty Type Layout."_view,
          "Keep the empty result as flow or infer from a Type with one value "
          "leaf."_view);
      return False;
    }

    type = &*initializer_type;
    initializer_linked = True;
    return True;
  }

  if (!selected_initializer->fits_into(**type)) {
    auto report = cursor.create_report(get_anchor());
    report << "Initializer for Field '"_view << get_name()
           << "' does not fit declared Type '"_view << (**type).get_name()
           << "'.\nSource produces: "_view;
    Language::Diagnostics::write_pack(report, *selected_initializer);
    report << "\nTarget accepts: "_view;
    Language::Diagnostics::write_layout(report, (**type).get_layout());
    report.get_hint()
        << "Change the initializer or declare the exact Type it produces."_view;
    return False;
  }

  initializer_linked = True;
  return True;
}

auto Language::Field::resolve_concept(View::Bytes route) const
    -> const Abstract& {
  if (route == "fold"_view) {
    return resolve_fold();
  }
  return get_host().resolve_lexical_context(route);
}

auto Language::Field::visit_concepts(ttx_named_abstract_callable* visitor) const
    -> void {
  Model::Addressable::visit_concepts(visitor);
  visit_concept(visitor, "fold"_view, resolve_fold());
}

auto Language::Field::link_declaration_constant(Cursor& cursor) const -> Bool {
  const Abstract& answer = resolve_fold();
  if (writability != Writability::Constant ||
      (&answer != &None::get_none() && Ttx::Concept::Constant::prove(answer))) {
    return True;
  }

  cursor.create_expression_error(
      get_anchor(),
      "Const Field initializer did not resolve to a compile-time value."_view,
      "Use only fully linked constant Expressions in a const Field."_view);
  return False;
}

auto Language::Field::validate_publication(Cursor& cursor) const -> Bool {
  if (!get_definition().is_published()) {
    return True;
  }

  const Model::Type& host = get_host();
  auto field_type = get_type().select<Model::Type>();
  BAIL_IF(!field_type);
  Bool reachable = type_reference.visit(
      [&]() {
        // An embedding Dialect has already selected this exact generated Type
        // edge from its public contract. Inferred Library Fields still prove
        // ordinary reachability through their real host.
        return Bool(generated || host.is_externally_reachable(*field_type));
      },
      [&](const TypeReference& reference) {
        Option<const Abstract&> selected;
        reference.resolve(host).visit(
            [&](const Abstract& resolved) { selected = resolved; },
            [](const TypeReference::Failure&) {});
        return Bool(selected && &selected->resolve() == &get_type());
      });
  if (reachable) {
    return True;
  }

  cursor.create_expression_error(
      get_type_anchor().visit(
          [&]() -> Option<Anchor> { return get_anchor(); },
          [](Anchor selected) -> Option<Anchor> { return selected; }),
      "Externally readable Field publishes an unreachable Type."_view,
      "Keep the Field private or publish its exact Type."_view);
  return False;
}

auto Language::Field::finalize_declaration(Cursor& cursor) -> Bool {
  initializer.visit(
      []() {}, [&](Model::Pack& selected) { selected.finalize(cursor); });
  return validate_publication(cursor);
}

auto Language::Field::resolve() const -> const Abstract& {
  if (!type) {
    return Unknown::get_unknown();
  }

  return *this;
}

auto Language::Field::get_initializer() const -> Option<const Model::Pack&> {
  return initializer.visit(
      []() -> Option<const Model::Pack&> { return {}; },
      [](const Model::Pack& selected) -> Option<const Model::Pack&> {
        return selected;
      });
}

auto Language::Field::resolve_fold() const -> const Abstract& {
  if (writability != Writability::Constant) {
    return None::get_none();
  }
  auto source = initializer.visit(
      []() -> Option<const Model::Pack&> { return {}; },
      [](const Model::Pack& selected) -> Option<const Model::Pack&> {
        return selected;
      });
  if (!source) {
    return Unknown::get_unknown();
  }
  if (source->get_layout().is_empty() && folded_input && folded_result) {
    return Abstract::from_abi(folded_result);
  }
  auto input_pack =
      query_folded_pack(domain, const_cast<Model::Pack&>(*source));
  if (!input_pack) {
    return query_fold(*source);
  }
  const Abstract& input = *input_pack->get_identity();
  if (folded_input == input.get_abi() && folded_result) {
    return Abstract::from_abi(folded_result);
  }
  auto field_type = get_type().select<Model::Type>();
  if (!field_type) {
    return None::get_none();
  }
  auto fitted = field_type->create_fitted(domain, *input_pack);
  const Abstract& result =
      fitted ? fold_answer(Option<Model::Pack&>(*fitted)) : input;
  if (!Ttx::Concept::Constant::prove(result)) {
    return None::get_none();
  }
  folded_input = input.get_abi();
  folded_result = result.get_abi();
  return result;
}
