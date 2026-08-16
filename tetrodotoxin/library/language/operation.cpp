// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/language/operation.hpp"

#include "ttx/concept/invalid.hpp"

using namespace Perimortem;
using namespace Tetrodotoxin::Library;

static auto retain_inputs(
    Memory::Allocator::Arena& domain,
    Core::View::Vector<Ttx::Concept::Reference<Language::Expression>>
        expressions)
    -> Memory::Managed::Vector<Ttx::Concept::Reference<Language::Expression>> {
  // Operation owns the mutable traversal inventory. Expressions borrow the
  // completed view below. No operand edge is copied into another graph.
  Memory::Managed::Vector<Ttx::Concept::Reference<Language::Expression>>
      retained(domain);
  retained.reset(expressions.get_size());
  for (const auto& expression : expressions) {
    retained.insert(expression);
  }
  return retained;
}

Language::Operation::Operation(
    Memory::Allocator::Arena& domain,
    Core::View::Vector<Ttx::Concept::Reference<Expression>> expressions,
    Core::Option<Ttx::Lexical::Anchor> anchor)
    : Expression(anchor),
      domain(domain),
      inputs(retain_inputs(domain, expressions)) {}

auto Language::Operation::get_type() const -> const Ttx::Concept::Abstract& {
  return result_type.visit(
      []() -> const Ttx::Concept::Abstract& {
        return Ttx::Concept::Invalid::get_invalid();
      },
      [](const Ttx::Concept::Reference<const Language::Model::Type>& selected)
          -> const Ttx::Concept::Abstract& { return selected.get(); });
}

auto Language::Operation::link(
    Ttx::Lexical::Cursor& cursor,
    const Ttx::Concept::Abstract& lexical_context,
    Core::Option<const Abstract&> access_scope) -> Bool {
  Bool failed = False;
  auto source_anchor = get_anchor();

  // Child order is authored evaluation order. Independent failures continue
  // so diagnostics retain that same order without making later graph edges
  // disappear from the source model.
  for (Count i = 0; i < inputs.get_size(); i++) {
    auto input = get_input(i);
    if (!input) {
      cursor.create_expression_error(
          source_anchor, "Operation contains an invalid Expression edge."_view,
          "Retain every authored operand as one Expression identity."_view);
      failed = True;
      continue;
    }

    // Operations preserve their caller's two contexts unchanged. Operand
    // nesting changes evaluation order, not lexical shadowing or host access.
    failed |= !input->link(cursor, lexical_context, access_scope);
  }

  BAIL_IF(failed);

  auto selected = select_type(lexical_context);
  if (!selected) {
    cursor.create_expression_error(
        source_anchor, "Operation rejects the linked operand Types."_view,
        "Use operands with the exact Types required by this operation."_view);
    return False;
  }

  if (result_type) {
    if (&result_type->get() == &*selected) {
      return True;
    }

    cursor.create_expression_error(
        source_anchor,
        "Operation result Type cannot change during linking."_view,
        "Keep one exact result Type on this authored operation."_view);
    return False;
  }

  result_type = Ttx::Concept::Reference<const Language::Model::Type>(*selected);
  return True;
}

auto Language::Operation::finalize(Ttx::Lexical::Cursor& cursor) -> void {
  // Operands are the canonical authored evaluation inventory. Finalize each
  // real producer in source order before asking this operation to cache its
  // own optional folded result.
  for (Ttx::Concept::Reference<Expression> input : inputs.get_view()) {
    input.get().finalize(cursor);
  }
  Expression::finalize(cursor);
}

auto Language::Operation::evaluate()
    -> Utility::Result<Core::Option<Model::Pack&>, Expression::Error> {
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
    return Core::Option<Model::Pack&>{};
  }

  return evaluate_constants(domain).visit(
      [](const Core::Option<Constant&>& constant)
          -> Utility::Result<Core::Option<Model::Pack&>, Expression::Error> {
        return constant.visit(
            []() -> Core::Option<Model::Pack&> { return {}; },
            [](Constant& selected) -> Core::Option<Model::Pack&> {
              return selected;
            });
      },
      [](const Expression::Error& error)
          -> Utility::Result<Core::Option<Model::Pack&>, Expression::Error> {
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

  return input->fold().visit(
      [](const Core::Option<Model::Pack&>& folded)
          -> Utility::Result<Core::Option<Expression&>, Expression::Error> {
        return folded.visit(
            []() -> Utility::Result<
                     Core::Option<Expression&>, Expression::Error> {
              return Core::Option<Expression&>{};
            },
            [](Model::Pack& selected)
                -> Utility::Result<
                    Core::Option<Expression&>, Expression::Error> {
              auto expression = selected.select<Expression>();
              return expression ? Core::Option<Expression&>(*expression)
                                : Core::Option<Expression&>{};
            });
      },
      [](const Expression::Error& error)
          -> Utility::Result<Core::Option<Expression&>, Expression::Error> {
        return error;
      });
}

auto Language::Operation::get_folded_input(Count index)
    -> Core::Option<Expression&> {
  auto input = get_input(index);
  BAIL_IF(!input);

  auto folded = input->get_folded();
  BAIL_IF(!folded);
  return folded->select<Expression>();
}

auto Language::Operation::get_input(Count index) -> Core::Option<Expression&> {
  BAIL_IF(index >= inputs.get_size());

  return inputs[index].get();
}

auto Language::Operation::get_input(Count index) const
    -> Core::Option<const Expression&> {
  BAIL_IF(index >= inputs.get_size());
  return inputs.get_view().get_data()[index].get();
}
