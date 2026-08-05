// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/language/operation.hpp"

using namespace Perimortem;
using namespace Tetrodotoxin::Library;

static auto retain_inputs(
    Memory::Allocator::Arena& domain,
    Core::View::Vector<Ttx::Concept::Reference<Language::Expression>>
        expressions)
    -> Core::Access::Vector<Ttx::Concept::Reference<Ttx::Concept::Abstract>> {
  auto retained =
      domain.reserve<Ttx::Concept::Reference<Ttx::Concept::Abstract>>(
          expressions.get_size());
  for (Count i = 0; i < expressions.get_size(); i++) {
    new (&retained[i]) Ttx::Concept::Reference<Ttx::Concept::Abstract>(
        expressions.get_data()[i].get());
  }

  return retained;
}

Language::Operation::Operation(
    Memory::Allocator::Arena& domain,
    Core::View::Vector<Ttx::Concept::Reference<Expression>> expressions)
    : inputs(retain_inputs(domain, expressions)),
      input_layout(inputs.get_view()) {}

auto Language::Operation::attempt_fold(
    Memory::Allocator::Arena& domain,
    Materializations& materializations) const
    -> Utility::Result<const Expression&, FoldError> {
  if (folded) {
    return folded->get();
  }

  const Ttx::Concept::Abstract& operation_type = get_type().resolve();
  if (!operation_type.is<Ttx::Model::Type>()) {
    return FoldError(FoldError::Type::InvalidOperationType, *this);
  }

  // Child Operations fold before the parent decides readiness. Each completed
  // replacement must preserve its exposed Type so folding cannot rewrite a
  // graph edge into a value the already checked parent did not accept.
  Bool all_constants = True;
  for (Count i = 0; i < inputs.get_size(); i++) {
    auto input = get_input(i);
    if (!input) {
      return FoldError(FoldError::Type::InvalidInput, *this);
    }

    Utility::Option<FoldError> child_error;
    const Ttx::Concept::Abstract& input_type = input->get_type().resolve();
    input->visit<Operation>(
        [&](const Operation& child) {
          auto child_result = child.attempt_fold(domain, materializations);
          child_result.visit(
              [&](const Expression& result) {
                const Ttx::Concept::Abstract& result_type =
                    result.get_type().resolve();
                if (!input_type.is<Ttx::Model::Type>() ||
                    !result_type.is<Ttx::Model::Type>() ||
                    &input_type != &result_type) {
                  child_error =
                      FoldError(FoldError::Type::ResultTypeMismatch, child);
                  return;
                }

                inputs[i] =
                    Ttx::Concept::Reference<Ttx::Concept::Abstract>(result);
              },
              [&](const FoldError& error) { child_error = error; });
        },
        [](const Ttx::Concept::Abstract&) {});
    if (child_error) {
      return *child_error;
    }

    auto replacement = get_input(i);
    if (!replacement) {
      return FoldError(FoldError::Type::InvalidInput, *this);
    }

    all_constants &= replacement->is<Constant>();
  }

  if (!all_constants) {
    return *this;
  }

  // The concrete owner sees only completed Constant inputs. Retaining a
  // replacement here makes repeated attempts observe one semantic result.
  auto evaluated = evaluate_constants(domain, materializations);
  return evaluated.visit(
      [&](const Expression& result)
          -> Utility::Result<const Expression&, FoldError> {
        const Ttx::Concept::Abstract& result_type = result.get_type().resolve();
        if (!result_type.is<Ttx::Model::Type>() ||
            &operation_type != &result_type) {
          return FoldError(FoldError::Type::ResultTypeMismatch, *this);
        }

        if (&result != this) {
          folded = Ttx::Concept::Reference<Expression>(result);
        }

        return result;
      },
      [](const FoldError& error)
          -> Utility::Result<const Expression&, FoldError> { return error; });
}

auto Language::Operation::get_input(Count index) const
    -> Utility::Option<const Expression&> {
  return input_layout.get_abstract(index).visit(
      []() -> Utility::Option<const Expression&> { return {}; },
      [](const Ttx::Concept::Abstract& input) {
        return input.visit<Expression>(
            [](const Expression& expression)
                -> Utility::Option<const Expression&> { return expression; },
            [](const Ttx::Concept::Abstract&)
                -> Utility::Option<const Expression&> { return {}; });
      });
}
