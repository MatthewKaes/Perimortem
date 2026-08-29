// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/library/language/expressions/initializer.hpp"

#include "tetrodotoxin/library/language/constant.hpp"
#include "tetrodotoxin/library/language/fold.hpp"
#include "ttx/bootstrap/concept/none.hpp"
#include "ttx/bootstrap/concept/unknown.hpp"

using namespace Perimortem;
using namespace Ttx::Concept;
using namespace Ttx::Model;
using namespace Tetrodotoxin::Library;

auto Language::Expressions::Initializer::create_authored(
    Memory::Allocator::Arena& domain,
    TypeReference target_reference,
    Model::Pack& arguments,
    Ttx::Lexical::Anchor anchor) -> Initializer& {
  return Expression::create_authored<Initializer>(
      domain, anchor,
      [&](Core::Option<Ttx::Lexical::Anchor> source) -> Initializer {
        return Initializer(domain, target_reference, arguments, source);
      });
}

auto Language::Expressions::Initializer::create_synthetic(
    Memory::Allocator::Arena& domain,
    const Language::Model::Type& type,
    Core::View::Vector<Model::Pack*> values) -> Initializer& {
  auto& arguments = Model::Pack::create_empty(domain);
  auto& completed = Model::Pack::create_group(domain, values);
  Initializer& initializer = Expression::create_synthetic<Initializer>(
      domain, [&](Core::Option<Ttx::Lexical::Anchor> source) -> Initializer {
        return Initializer(domain, {}, arguments, source);
      });
  initializer.expected_type = &type;
  initializer.completed_values = &completed;
  return initializer;
}

auto Language::Expressions::Initializer::create_provider(
    Memory::Allocator::Arena& domain,
    const Language::Model::Type& type,
    Language::Model::Pack& arguments) -> Initializer& {
  Initializer& initializer = Expression::create_synthetic<Initializer>(
      domain, [&](Core::Option<Ttx::Lexical::Anchor> source) -> Initializer {
        return Initializer(domain, {}, arguments, source);
      });
  initializer.expected_type = &type;
  initializer.provider = True;
  return initializer;
}

Language::Expressions::Initializer::Initializer(
    Memory::Allocator::Arena& domain,
    Core::Option<TypeReference> target_reference,
    Language::Model::Pack& arguments,
    Core::Option<Ttx::Lexical::Anchor> anchor)
    : Expression(anchor),
      domain(domain),
      target_reference(target_reference),
      arguments(arguments) {}

auto Language::Expressions::Initializer::get_type() const -> const Abstract& {
  return expected_type.visit(
      []() -> const Abstract& { return Unknown::get_unknown(); },
      [](const Language::Model::Type* selected) -> const Abstract& {
        return *selected;
      });
}

auto Language::Expressions::Initializer::fits(
    const Ttx::Model::Type& target) const -> Bool {
  return expected_type && *expected_type == &target;
}

auto Language::Expressions::Initializer::finalize(Ttx::Lexical::Cursor& cursor)
    -> void {
  // The initializer owns the complete argument flow. Finalize its real Pack in
  // source order before folding the initializer node itself. No second
  // expression inventory exists beside the Pack's canonical Layout.
  arguments.finalize(cursor);
  completed_values.visit(
      []() {}, [&](Model::Pack* values) { values->finalize(cursor); });
  Expression::finalize(cursor);
}

auto Language::Expressions::Initializer::get_completed_values() const
    -> Core::Option<const Model::Pack&> {
  return completed_values.visit(
      []() -> Core::Option<const Model::Pack&> { return {}; },
      [](Model::Pack* selected) -> Core::Option<const Model::Pack&> {
        return *selected;
      });
}

auto Language::Expressions::Initializer::resolve_concept(
    Core::View::Bytes name) const -> const Abstract& {
  if (name != "fold"_view) {
    return Expression::resolve_concept(name);
  }
  return const_cast<Initializer&>(*this).evaluate_fold().visit(
      [](const Core::Option<Model::Pack&>& result) -> const Abstract& {
        return fold_answer(result);
      },
      [](const Expression::Error&) -> const Abstract& {
        return Ttx::Concept::None::get_none();
      });
}

auto Language::Expressions::Initializer::visit_concepts(
    ttx_named_abstract_callable* visitor) const -> void {
  Expression::visit_concepts(visitor);
  visit_concept(visitor, "fold"_view, resolve_concept("fold"_view));
}

auto Language::Expressions::Initializer::evaluate_fold()
    -> Utility::Result<Core::Option<Model::Pack&>, Expression::Error> {
  return completed_values.visit(
      []() -> Utility::Result<Core::Option<Model::Pack&>, Expression::Error> {
        return Core::Option<Model::Pack&>();
      },
      [&](Model::Pack* selected)
          -> Utility::Result<Core::Option<Model::Pack&>, Expression::Error> {
        auto representation = query_folded_pack(domain, *selected);
        auto identity = representation ? representation->get_identity()
                                       : Core::Option<const Abstract&>();
        return identity && Ttx::Concept::Constant::prove(*identity) &&
                       expected_type &&
                       representation->fits_into(**expected_type)
                   ? representation
                   : Core::Option<Model::Pack&>();
      });
}

auto Language::Expressions::Initializer::link(
    Ttx::Lexical::Cursor& cursor,
    const Abstract& lexical_context,
    Core::Option<const Abstract&> access_scope) -> Bool {
  if (expected_type && provider) {
    BAIL_IF(!arguments.link(cursor, lexical_context, access_scope));
    return Expression::link(cursor, lexical_context, access_scope);
  }

  if (expected_type && completed_values) {
    BAIL_IF(!(*completed_values)->link(cursor, lexical_context, access_scope));
    return Expression::link(cursor, lexical_context, access_scope);
  }

  if (!target_reference) {
    cursor.create_expression_error(
        get_anchor(), "Initializer lost its authored Type reference."_view,
        "Retain `new[Type]` as one complete source expression."_view);
    return False;
  }

  auto selected = target_reference->resolve_authored(cursor, lexical_context);
  BAIL_IF(!selected);
  auto target = selected->select<Language::Model::Type>();
  if (!target || target->get_layout().is_empty()) {
    cursor.create_expression_error(
        target_reference->get_anchor(),
        "Initializer requires one nonempty Library Type."_view,
        "Name a completed value Type in `new[Type]`."_view);
    return False;
  }

  if (expected_type) {
    if (*expected_type == &*target) {
      return True;
    }

    cursor.create_expression_error(
        get_anchor(), "Initializer cannot change its expected Type."_view,
        "Keep the authored initializer on its original declaration."_view);
    return False;
  }

  BAIL_IF(!arguments.link(cursor, lexical_context, access_scope));
  // Object construction is the first semantic consumer of these arguments.
  // Keep Type results usable as receivers while refusing them as Field values.
  if (!arguments.is_complete()) {
    cursor.create_expression_error(
        get_anchor(), "Initializer arguments did not produce value flow."_view,
        "Supply instance values and keep Type results as access receivers."_view);
    return False;
  }

  const Layout& inputs = arguments.get_layout();
  if (inputs.is_empty()) {
    auto value = target->create_default(cursor.get_arena());
    if (!value) {
      cursor.create_expression_error(
          get_anchor(), "Initializer could not create the Type default."_view,
          "Use a completed source value Type with a terminating default."_view);
      return False;
    }

    Model::Pack* completed = &*value;
    auto aggregate = value->select_identity<Initializer>();
    if (aggregate && !aggregate->get_anchor()) {
      auto aggregate_values = aggregate->get_completed_values();
      if (aggregate_values) {
        // A local aggregate exposes the completed Field values owned by Type.
        // A restored aggregate instead remains the real provider Initializer
        // so lowering can invoke its provider without inventing those Fields.
        completed = &const_cast<Model::Pack&>(*aggregate_values);
      }
    }

    expected_type = &*target;
    completed_values = completed;
    BAIL_IF(!completed->link(cursor, lexical_context, access_scope));
    return Expression::link(cursor, lexical_context, access_scope);
  }

  auto completed =
      target->create_supplied(cursor, arguments, access_scope, get_anchor());
  BAIL_IF(!completed);
  BAIL_IF(!completed->link(cursor, lexical_context, access_scope));
  if (!completed->fits_into(*target)) {
    cursor.create_expression_error(
        get_anchor(),
        "Initializer completed values do not fit the selected Type Layout."_view,
        "Keep the Type's construction result consistent with its Layout."_view);
    return False;
  }

  expected_type = &*target;
  completed_values = &*completed;
  return True;
}
