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
    new (&retained[i])
        Ttx::Concept::Reference<Ttx::Concept::Abstract>(expressions[i].get());
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

  // Child Operations fold before the parent decides readiness. Each completed
  // replacement keeps its authored position while failure remains explicit.
  Bool all_constants = True;
  for (Count i = 0; i < inputs.get_size(); i++) {
    const Ttx::Concept::Abstract& input = inputs[i].get();
    FoldError child_error = FoldError::Unknown;
    Bool child_failed = input.visit<Operation>(
        [&](const Operation& child) {
          auto child_result = child.attempt_fold(domain, materializations);
          return child_result.visit(
              [&](const Expression& result) {
                inputs[i] =
                    Ttx::Concept::Reference<Ttx::Concept::Abstract>(result);
                return False;
              },
              [&](FoldError error) {
                child_error = error;
                return True;
              });
        },
        [](const Ttx::Concept::Abstract&) { return False; });
    if (child_failed) {
      return child_error;
    }

    all_constants &= inputs[i].get().is<Constant>();
  }

  if (!all_constants) {
    return static_cast<const Expression&>(*this);
  }

  // The concrete owner sees only completed Constant inputs. Retaining a
  // replacement here makes repeated attempts observe one semantic result.
  auto evaluated = evaluate_constants(domain, materializations);
  return evaluated.visit(
      [&](const Expression& result)
          -> Utility::Result<const Expression&, FoldError> {
        if (&result != this) {
          folded = Ttx::Concept::Reference<Expression>(result);
        }

        return result;
      },
      [](FoldError error) -> Utility::Result<const Expression&, FoldError> {
        return error;
      });
}
