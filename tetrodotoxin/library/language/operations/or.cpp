// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/language/operations/or.hpp"

#include "perimortem/core/static/vector.hpp"

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

static auto select_flag(Language::Expression& expression)
    -> Utility::Option<const Language::Constants::Flag&> {
  return expression.visit<Language::Constants::Flag>(
      [](const Language::Constants::Flag& selected)
          -> Utility::Option<const Language::Constants::Flag&> {
        return selected;
      },
      [](const Abstract&) -> Utility::Option<const Language::Constants::Flag&> {
        return {};
      });
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
    Expression& left) -> Utility::Option<Expression&> {
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

auto Language::Operations::Or::create_authored(
    Memory::Allocator::Arena& domain,
    Materializations& materializations,
    Expression& left,
    Expression& right,
    Anchor anchor) -> Or& {
  return Expression::create_authored<Or>(
      domain, anchor, [&](auto source) -> Or {
        return Or(domain, materializations, left, right, source);
      });
}

auto Language::Operations::Or::create_synthetic(
    Memory::Allocator::Arena& domain,
    Materializations& materializations,
    Expression& left,
    Expression& right) -> Or& {
  return Expression::create_synthetic<Or>(domain, [&](auto source) -> Or {
    return Or(domain, materializations, left, right, source);
  });
}

Language::Operations::Or::Or(
    Memory::Allocator::Arena& domain,
    Materializations& materializations,
    Expression& left,
    Expression& right,
    Utility::Option<Anchor> anchor)
    : Operation(
          domain,
          materializations,
          Core::Static::Vector<Ttx::Concept::Reference<Expression>, 2>{
            {left, right}},
          anchor) {}

auto Language::Operations::Or::get_documentation() const
    -> const Documentation& {
  return Documentation::get_empty();
}

auto Language::Operations::Or::select_type(Materializations&) const
    -> Utility::Option<const Type&> {
  auto left = get_input(0);
  auto right = get_input(1);
  if (!left || !right) {
    return {};
  }

  return select_result_type(*left, *right)
      .visit<Type>(
          [](const Type& type) -> Utility::Option<const Type&> { return type; },
          [](const Abstract&) -> Utility::Option<const Type&> { return {}; });
}

auto Language::Operations::Or::reaches_next_input(
    Count folded_input,
    const Expression& projection) const -> Bool {
  if (folded_input != 0) {
    return True;
  }

  auto left = projection.visit<Constants::Flag>(
      [](const Constants::Flag& selected)
          -> Utility::Option<const Constants::Flag&> { return selected; },
      [](const Abstract&) -> Utility::Option<const Constants::Flag&> {
        return {};
      });
  return !left || !left->get_value();
}

auto Language::Operations::Or::evaluate_constants(
    Memory::Allocator::Arena& domain,
    Materializations&)
    -> Utility::Result<Utility::Option<Constant&>, Expression::Error> {
  auto authored_left = get_input(0);
  auto authored_right = get_input(1);
  auto left = get_folded_input(0);
  if (!authored_left || !authored_right || !left) {
    return Expression::Error(Expression::Error::Type::InvalidInput, *this);
  }

  // True closes disjunction before the right edge matters. False reaches the
  // right projection and keeps any failure attached to that authored input.
  auto left_value = select_flag(*left);
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

  auto right_value = select_flag(*right);
  if (!right_value) {
    return Expression::Error(
        Expression::Error::Type::InvalidConstant, *authored_right);
  }

  return make_result(domain, right_value->get_value());
}
