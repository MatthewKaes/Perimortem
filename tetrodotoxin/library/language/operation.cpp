// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/library/language/operation.hpp"

#include "tetrodotoxin/library/language/diagnostics.hpp"
#include "ttx/concept/none.hpp"
#include "ttx/concept/unknown.hpp"

using namespace Perimortem;
using namespace Tetrodotoxin::Library;

static auto retain_inputs(
    Memory::Allocator::Arena& domain,
    Core::View::Vector<Language::Model::Pack*> expressions)
    -> Memory::Managed::Vector<Language::Model::Pack*> {
  // Operation owns the mutable traversal inventory. Packs borrow the
  // completed view below. No operand edge is copied into another graph.
  Memory::Managed::Vector<Language::Model::Pack*> retained(domain);
  retained.reset(expressions.get_size());
  for (const auto& expression : expressions) {
    retained.insert(expression);
  }
  return retained;
}

Language::Operation::Operation(
    Memory::Allocator::Arena& domain,
    Core::View::Vector<Model::Pack*> expressions,
    Core::Option<Ttx::Lexical::Anchor> anchor)
    : Expression(anchor),
      domain(domain),
      inputs(retain_inputs(domain, expressions)),
      folded_inputs(domain) {}

auto Language::Operation::get_type() const -> const Ttx::Concept::Abstract& {
  return result_type.visit(
      []() -> const Ttx::Concept::Abstract& {
        return Ttx::Concept::Unknown::get_unknown();
      },
      [](const Language::Model::Type* selected)
          -> const Ttx::Concept::Abstract& { return *selected; });
}

auto Language::Operation::resolve_concept(Core::View::Bytes name) const
    -> const Abstract& {
  if (name != "fold"_view) {
    return Expression::resolve_concept(name);
  }
  auto result = const_cast<Operation&>(*this).evaluate_fold();
  return result.visit(
      [](const Core::Option<Model::Pack&>& folded) -> const Abstract& {
        if (!folded) {
          return Ttx::Concept::None::get_none();
        }
        auto identity = folded->get_identity();
        return identity && Ttx::Concept::Constant::prove(*identity)
                   ? *identity
                   : static_cast<const Abstract&>(
                         Ttx::Concept::None::get_none());
      },
      [](const Expression::Error&) -> const Abstract& {
        return Ttx::Concept::None::get_none();
      });
}

auto Language::Operation::visit_concepts(
    ttx_named_abstract_callable* visitor) const -> void {
  Expression::visit_concepts(visitor);
  visit_concept(visitor, "fold"_view, resolve_concept("fold"_view));
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
    Model::Pack& input = *inputs[i];

    // Operations preserve their caller's two contexts unchanged. Operand
    // nesting changes evaluation order, not lexical shadowing or host access.
    failed |= !input.link(cursor, lexical_context, access_scope);
  }

  BAIL_IF(failed);

  auto selected = select_type(lexical_context);
  if (!selected) {
    auto report = cursor.create_report(source_anchor);
    report << "Operation '"_view << get_name()
           << "' rejects operand Types ["_view;
    for (Count index = 0; index < inputs.get_size(); index++) {
      if (index != 0) {
        report << ", "_view;
      }
      Language::Diagnostics::write_type(report, inputs.at(index)->get_type());
    }
    report << "]."_view;
    report.get_hint()
        << "Use the exact matching operand Types accepted by '"_view
        << get_name() << "'."_view;
    return False;
  }

  if (result_type) {
    if (*result_type == &*selected) {
      return True;
    }

    auto report = cursor.create_report(source_anchor);
    report << "Internal semantic error: Operation '"_view << get_name()
           << "' changed result Type from '"_view << (**result_type).get_name()
           << "' to '"_view << selected->get_name() << "'."_view;
    report.get_hint()
        << "The source is valid; report this unstable linking result."_view;
    return False;
  }

  result_type = &*selected;
  return True;
}

auto Language::Operation::finalize(Ttx::Lexical::Cursor& cursor) -> void {
  // Operands are the canonical authored evaluation inventory. Finalize each
  // real producer in source order before asking this operation to cache its
  // own optional folded result.
  for (Model::Pack* input : inputs.get_view()) {
    input->finalize(cursor);
  }
  Expression::finalize(cursor);
}

auto Language::Operation::evaluate_fold()
    -> Utility::Result<Core::Option<Model::Pack&>, Expression::Error> {
  if (!result_type) {
    return Core::Option<Model::Pack&>{};
  }

  Memory::Managed::Vector<const Abstract*> reached(domain);
  reached.reset(inputs.get_size());
  Bool all_reached_folded = True;
  for (Count i = 0; i < inputs.get_size(); i++) {
    auto child_result = fold_input(i);
    Core::Option<Constant&> child_fold;
    Core::Option<Expression::Error> child_error;
    child_result.visit(
        [&](const Core::Option<Constant&>& selected) { child_fold = selected; },
        [&](const Expression::Error& error) { child_error = error; });
    if (child_error) {
      return *child_error;
    }

    all_reached_folded &= bool(child_fold);
    if (child_fold) {
      reached.insert(&static_cast<const Abstract&>(*child_fold));
    }
    if (i + 1 < inputs.get_size() && child_fold &&
        !reaches_next_input(i, *child_fold)) {
      break;
    }
  }

  if (!all_reached_folded) {
    return Core::Option<Model::Pack&>{};
  }

  Bool same_inputs =
      folded_result && folded_inputs.get_size() == reached.get_size();
  for (Count index = 0; same_inputs && index < reached.get_size(); index++) {
    same_inputs &= folded_inputs.at(index) == reached.at(index);
  }
  if (same_inputs) {
    auto cached = Model::Pack::from(const_cast<Abstract&>(*folded_result));
    return cached ? Core::Option<Model::Pack&>(*cached)
                  : Core::Option<Model::Pack&>();
  }

  return evaluate_constants(domain).visit(
      [&](const Core::Option<Tetrodotoxin::Library::Language::Constant&>&
              constant)
          -> Utility::Result<Core::Option<Model::Pack&>, Expression::Error> {
        if (!constant) {
          return Core::Option<Model::Pack&>{};
        }
        if (&constant->get_type().resolve() != &(**result_type).resolve()) {
          return Expression::Error(
              Expression::Error::Type::ResultTypeMismatch, *this);
        }
        folded_inputs.reset(reached.get_size());
        for (const Abstract* input : reached.get_view()) {
          folded_inputs.insert(input);
        }
        folded_result = &static_cast<const Abstract&>(*constant);
        return Core::Option<Model::Pack&>(*constant);
      },
      [](const Expression::Error& error)
          -> Utility::Result<Core::Option<Model::Pack&>, Expression::Error> {
        return error;
      });
}

auto Language::Operation::reaches_next_input(Count, const Constant&) const
    -> Bool {
  return True;
}

auto Language::Operation::fold_input(Count index)
    -> Utility::Result<Core::Option<Constant&>, Expression::Error> {
  if (index >= inputs.get_size()) {
    return Expression::Error(Expression::Error::Type::InvalidInput, *this);
  }

  Model::Pack& input = *inputs[index];
  auto constant = input.select_identity<Constant>();
  if (constant) {
    return *constant;
  }

  auto expression = input.select_identity<Expression>();
  if (!expression) {
    return Expression::Error::from_pack(
        Expression::Error::Type::InvalidInput, input);
  }
  const Abstract& folded = expression->resolve_concept("fold"_view);
  auto selected = const_cast<Abstract&>(folded).select<Constant>();
  return selected ? Core::Option<Constant&>(*selected)
                  : Core::Option<Constant&>();
}

auto Language::Operation::get_folded_input(Count index)
    -> Core::Option<Constant&> {
  BAIL_IF(index >= inputs.get_size());

  Model::Pack& input = *inputs[index];
  auto constant = input.select_identity<Constant>();
  if (constant) {
    return *constant;
  }
  auto expression = input.select_identity<Expression>();
  BAIL_IF(!expression);
  const Abstract& folded = expression->resolve_concept("fold"_view);
  auto selected = const_cast<Abstract&>(folded).select<Constant>();
  return selected ? Core::Option<Constant&>(*selected)
                  : Core::Option<Constant&>();
}
