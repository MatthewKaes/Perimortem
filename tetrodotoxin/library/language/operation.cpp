// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/language/operation.hpp"

#include "ttx/concept/invalid.hpp"
#include "ttx/model/addressable.hpp"

using namespace Perimortem;
using namespace Tetrodotoxin::Library;

Language::Operation::Operation(
    Memory::Allocator::Arena& domain,
    Materializations& materializations,
    Core::View::Vector<Ttx::Concept::Reference<Expression>> expressions,
    Core::Option<Ttx::Lexical::Anchor> anchor)
    : Expression(anchor),
      domain(domain),
      materializations(materializations),
      inputs(domain),
      input_layout(inputs) {
  inputs.reset(expressions.get_size());
  for (Count i = 0; i < expressions.get_size(); i++) {
    inputs.insert(expressions.get_data()[i]);
  }
}

static auto selects_fitting_type(
    const Ttx::Concept::Abstract& value,
    const Language::Expression& expression) -> Bool {
  const Ttx::Concept::Abstract& selected = value.visit<Ttx::Model::Addressable>(
      [](const Ttx::Model::Addressable& addressable)
          -> const Ttx::Concept::Abstract& {
        return addressable.get_type().resolve();
      },
      [](const Ttx::Concept::Abstract& abstract)
          -> const Ttx::Concept::Abstract& { return abstract.resolve(); });
  return selected.visit<Ttx::Model::Type>(
      [&expression](const Ttx::Model::Type& type) {
        return expression.fits(type);
      },
      [](const Ttx::Concept::Abstract&) { return False; });
}

constexpr auto Language::Operation::InputLayout::get_abstract(Count index) const
    -> Core::Option<const Ttx::Concept::Abstract&> {
  BAIL_IF(index >= inputs.get_size());

  return inputs.at(index).get();
}

auto Language::Operation::InputLayout::fits_at(
    const Ttx::Concept::Layout& target,
    Count target_offset) const -> Bool {
  BAIL_IF(!has_target_segment(target, target_offset));

  for (Count i = 0; i < get_size(); i++) {
    const Language::Expression& source = inputs.at(i).get();
    Bool entry_fits = target.get_abstract(target_offset + i)
                          .visit(
                              []() { return False; },
                              [&source](const Ttx::Concept::Abstract& target) {
                                return selects_fitting_type(target, source);
                              });
    BAIL_IF(!entry_fits);
  }

  return True;
}

auto Language::Operation::InputLayout::get_fitted_at(
    const Ttx::Concept::Layout& target,
    Count target_offset,
    Count target_index) const
    -> Utility::Result<const Ttx::Concept::Abstract&, Errors> {
  if (target_index >= get_size()) {
    return Errors::IndexOutOfBounds;
  }
  if (!has_target_segment(target, target_offset)) {
    return Errors::SizeMismatch;
  }
  if (!fits_at(target, target_offset)) {
    return Errors::IncompatibleFit;
  }

  return inputs.at(target_index).get();
}

auto Language::Operation::get_type() const -> const Ttx::Concept::Abstract& {
  return result_type.visit(
      []() -> const Ttx::Concept::Abstract& {
        return Ttx::Concept::Invalid::get_invalid();
      },
      [](const Ttx::Concept::Reference<const Ttx::Model::Type>& selected)
          -> const Ttx::Concept::Abstract& { return selected.get(); });
}

auto Language::Operation::link(
    Tetrodotoxin::Language::Monograph& source,
    const Ttx::Concept::Abstract& lexical_context,
    Materializations& materializations,
    Core::Option<const Ttx::Model::Type&> access_scope) -> Bool {
  Bool failed = False;
  auto source_anchor = get_anchor();

  if (&materializations != &this->materializations) {
    source.report(
        source_anchor,
        "Operation cannot link through another Materializations owner."_view,
        "Reuse the graph inventory retained when this operation was built."_view);
    return False;
  }

  // Child order is authored evaluation order. Independent failures continue
  // so diagnostics retain that same order without making later graph edges
  // disappear from the source model.
  for (Count i = 0; i < inputs.get_size(); i++) {
    auto input = get_input(i);
    if (!input) {
      source.report(
          source_anchor, "Operation contains an invalid Expression edge."_view,
          "Retain every authored operand as one Expression identity."_view);
      failed = True;
      continue;
    }

    // Operations preserve their caller's two contexts unchanged. Operand
    // nesting changes evaluation order, not lexical shadowing or host access.
    failed |=
        !input->link(source, lexical_context, materializations, access_scope);
  }

  BAIL_IF(failed);

  auto selected = select_type(this->materializations);
  if (!selected) {
    source.report(
        source_anchor, "Operation rejects the linked operand Types."_view,
        "Use operands with the exact Types required by this operation."_view);
    return False;
  }

  if (result_type) {
    if (&result_type->get() == &*selected) {
      return True;
    }

    source.report(
        source_anchor,
        "Operation result Type cannot change during linking."_view,
        "Keep one exact result Type on this authored operation."_view);
    return False;
  }

  result_type = Ttx::Concept::Reference<const Ttx::Model::Type>(*selected);
  return True;
}

auto Language::Operation::fold_uncached()
    -> Utility::Result<Core::Option<Expression&>, Expression::Error> {
  Bool all_reached_folded = True;
  for (Count i = 0; i < inputs.get_size(); i++) {
    auto child_result = fold_input(i);
    Core::Option<Expression&> child_fold;
    Core::Option<Expression::Error> child_error;
    child_result.visit(
        [&](const Core::Option<Expression&>& selected) {
          child_fold = selected;
        },
        [&](const Expression::Error& error) { child_error = error; });
    if (child_error) {
      return *child_error;
    }

    all_reached_folded &= bool(child_fold);
    if (i + 1 < inputs.get_size() && child_fold &&
        !reaches_next_input(i, *child_fold)) {
      break;
    }
  }

  if (!all_reached_folded) {
    return Core::Option<Expression&>{};
  }

  auto evaluated = evaluate_constants(domain, materializations);
  return evaluated.visit(
      [](const Core::Option<Constant&>& selected)
          -> Utility::Result<Core::Option<Expression&>, Expression::Error> {
        return selected.visit(
            []() -> Core::Option<Expression&> { return {}; },
            [](Constant& constant) -> Core::Option<Expression&> {
              return constant;
            });
      },
      [](const Expression::Error& error)
          -> Utility::Result<Core::Option<Expression&>, Expression::Error> {
        return error;
      });
}

auto Language::Operation::reaches_next_input(Count, const Expression&) const
    -> Bool {
  return True;
}

auto Language::Operation::fold_input(Count index)
    -> Utility::Result<Core::Option<Expression&>, Expression::Error> {
  auto input = get_input(index);
  if (!input) {
    return Expression::Error(Expression::Error::Type::InvalidInput, *this);
  }

  return input->fold();
}

auto Language::Operation::get_folded_input(Count index)
    -> Core::Option<Expression&> {
  auto input = get_input(index);
  BAIL_IF(!input);

  return input->get_folded();
}

auto Language::Operation::get_input(Count index) -> Core::Option<Expression&> {
  BAIL_IF(index >= inputs.get_size());

  return inputs[index].get();
}

auto Language::Operation::get_input(Count index) const
    -> Core::Option<const Expression&> {
  return input_layout.get_abstract(index).visit(
      []() -> Core::Option<const Expression&> { return {}; },
      [](const Ttx::Concept::Abstract& input) {
        return input.select<Expression>();
      });
}
