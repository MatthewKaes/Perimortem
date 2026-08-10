// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/language/operations/range.hpp"

#include "perimortem/core/static/vector.hpp"

#include "tetrodotoxin/library/language/generics/range.hpp"
#include "tetrodotoxin/library/language/parser/expression.hpp"
#include "ttx/concept/invalid.hpp"
#include "ttx/model/types/signed.hpp"
#include "ttx/model/types/unsigned.hpp"

using namespace Perimortem;
using namespace Tetrodotoxin::Library;
using namespace Ttx::Concept;
using namespace Ttx::Lexical;
using namespace Ttx::Model;

auto Language::Operations::Range::parse(
    Memory::Allocator::Arena& domain,
    Materializations& materializations,
    Cursor& cursor,
    const Abstract& source_context,
    Expression& left) -> Core::Option<Expression&> {
  auto transaction = cursor.branch();
  Token opening = transaction.consume();
  auto right = Language::Parser::Expression::parse_operand(
      domain, materializations, transaction, source_context,
      Code::Type::RangeOp);
  Span span(opening, transaction.peek(-1));
  if (!right) {
    transaction.create_expression_error(
        span, "Range has a malformed right endpoint."_view,
        "Use one complete integer Expression after `...`."_view);
    return {};
  }
  if (transaction.matches(Code::Type::RangeOp)) {
    transaction.create_expression_error(
        Span(opening, transaction.current()),
        "Range accepts exactly two endpoints."_view,
        "Finish one Range before starting another Expression."_view);
    return {};
  }

  const auto& left_anchor = left.get_anchor();
  const auto& right_anchor = right->get_anchor();
  if (!left_anchor || !right_anchor) {
    transaction.create_expression_error(
        span, "Range requires authored endpoint Anchors."_view);
    return {};
  }

  Anchor anchor = Anchor::create(
      opening, left_anchor->get_span(), right_anchor->get_span());
  Range& range =
      create_authored(domain, materializations, left, *right, anchor);
  cursor.join(transaction);
  return range;
}

auto Language::Operations::Range::create_authored(
    Memory::Allocator::Arena& domain,
    Materializations& materializations,
    Expression& left,
    Expression& right,
    Anchor anchor) -> Range& {
  return Expression::create_authored<Range>(
      domain, anchor, [&](auto source) -> Range {
        return Range(domain, materializations, left, right, source);
      });
}

auto Language::Operations::Range::create_synthetic(
    Memory::Allocator::Arena& domain,
    Materializations& materializations,
    Expression& left,
    Expression& right) -> Range& {
  return Expression::create_synthetic<Range>(domain, [&](auto source) -> Range {
    return Range(domain, materializations, left, right, source);
  });
}

Language::Operations::Range::Range(
    Memory::Allocator::Arena& domain,
    Materializations& materializations,
    Expression& left,
    Expression& right,
    Core::Option<Anchor> anchor)
    : Operation(
          domain,
          materializations,
          Core::Static::Vector<Ttx::Concept::Reference<Expression>, 2>{
            {left, right}},
          anchor) {}

auto Language::Operations::Range::get_documentation() const
    -> const Documentation& {
  return Documentation::get_empty();
}

auto Language::Operations::Range::select_type(
    Materializations& materializations) const -> Core::Option<const Type&> {
  auto left = get_input(0);
  auto right = get_input(1);
  if (!left || !right) {
    return {};
  }

  const Abstract& left_type = left->get_type().resolve();
  const Abstract& right_type = right->get_type().resolve();
  auto element = left_type.select<Type>();
  if (!element || &left_type != &right_type ||
      (!left_type.is<Ttx::Model::Types::Signed>() &&
       !left_type.is<Ttx::Model::Types::Unsigned>())) {
    return {};
  }

  Core::Static::Vector<Generic::Argument, 1> arguments = {{
    Generic::Argument(*element),
  }};
  return materializations.materialize(
      Generics::Range::get_formula(), arguments.get_view());
}

auto Language::Operations::Range::evaluate_constants(
    Memory::Allocator::Arena&,
    Materializations&)
    -> Utility::Result<Core::Option<Constant&>, Expression::Error> {
  return Core::Option<Constant&>{};
}
