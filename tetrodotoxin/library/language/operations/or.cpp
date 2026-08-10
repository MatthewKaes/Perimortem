// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/language/operations/or.hpp"

#include "tetrodotoxin/library/dialect.hpp"
#include "tetrodotoxin/library/language/constants/false.hpp"
#include "tetrodotoxin/library/language/constants/flag.hpp"
#include "tetrodotoxin/library/language/constants/true.hpp"
#include "tetrodotoxin/library/language/parser/expression.hpp"
#include "ttx/concept/invalid.hpp"

using namespace Perimortem;
using namespace Tetrodotoxin::Library;
using namespace Ttx::Concept;
using namespace Ttx::Lexical;
using namespace Ttx::Model;

static auto select_result_type(
    const Language::Expression& left,
    const Language::Expression& right) -> const Abstract& {
  const Abstract& selected_left = left.get_type().resolve();
  const Abstract& selected_right = right.get_type().resolve();
  if (&selected_left != &Dialect::get_bool() ||
      &selected_right != &Dialect::get_bool()) {
    return Invalid::get_invalid();
  }

  return selected_left;
}

static auto make_result(Memory::Allocator::Arena& domain, Bool value)
    -> Language::Constant& {
  if (value) {
    return Language::Constants::True::create_synthetic(
        domain, Dialect::get_bool());
  }

  return Language::Constants::False::create_synthetic(
      domain, Dialect::get_bool());
}

auto Language::Operations::Or::parse(
    Memory::Allocator::Arena& domain,
    Materializations& materializations,
    Cursor& cursor,
    const Abstract& source_context,
    Expression& left) -> Core::Option<Expression&> {
  auto transaction = cursor.branch();
  Token opening = transaction.consume();
  auto right = Language::Parser::Expression::parse_operand(
      domain, materializations, transaction, source_context, Code::Type::OrOp);
  Span span(opening, transaction.peek(-1));
  if (!right) {
    transaction.create_expression_error(
        span, "Or has a malformed right operand."_view,
        "Use a complete Bool Expression after `|`."_view);
    return {};
  }

  const auto& left_anchor = left.get_anchor();
  const auto& right_anchor = right->get_anchor();
  if (!left_anchor || !right_anchor) {
    transaction.create_expression_error(
        span, "Or requires authored operand Anchors."_view);
    return {};
  }

  auto anchor = Anchor::create(
      opening, left_anchor->get_span(), right_anchor->get_span());
  auto& operation =
      create_authored(domain, materializations, left, *right, anchor);
  cursor.join(transaction);
  return operation;
}

TTX_BINARY_OP(Or);

auto Language::Operations::Or::get_documentation() const
    -> const Documentation& {
  return Documentation::get_empty();
}

auto Language::Operations::Or::select_type(Materializations&) const
    -> Core::Option<const Type&> {
  auto left = get_input(0);
  auto right = get_input(1);
  if (!left || !right) {
    return {};
  }

  return select_result_type(*left, *right).select<Type>();
}

auto Language::Operations::Or::reaches_next_input(
    Count folded_input,
    const Expression& folded) const -> Bool {
  if (folded_input != 0) {
    return True;
  }

  auto left = folded.select<Constants::Flag>();
  return !left || !left->get_value();
}

auto Language::Operations::Or::evaluate_constants(
    Memory::Allocator::Arena& domain,
    Materializations&)
    -> Utility::Result<Core::Option<Constant&>, Expression::Error> {
  auto authored_left = get_input(0);
  auto authored_right = get_input(1);
  auto left = get_folded_input(0);
  if (!authored_left || !authored_right || !left) {
    return Expression::Error(Expression::Error::Type::InvalidInput, *this);
  }

  // True closes disjunction before the right edge matters. False reaches the
  // right input and keeps any failure attached to that authored Expression.
  auto left_value = left->select<Constants::Flag>();
  if (!left_value) {
    return Expression::Error(
        Expression::Error::Type::InvalidConstant, *authored_left);
  }

  if (left_value->get_value()) {
    return make_result(domain, True);
  }

  auto right = get_folded_input(1);
  if (!right) {
    return Expression::Error(Expression::Error::Type::InvalidInput, *this);
  }

  auto right_value = right->select<Constants::Flag>();
  if (!right_value) {
    return Expression::Error(
        Expression::Error::Type::InvalidConstant, *authored_right);
  }

  return make_result(domain, right_value->get_value());
}
