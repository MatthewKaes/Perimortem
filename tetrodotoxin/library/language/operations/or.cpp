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

TTX_TRANSACTIONAL_BINARY_PARSE(
    Or,
    Or,
    "Or has a malformed right operand."_view,
    "Use a complete Bool Expression after `or`."_view);

TTX_BINARY_OP(Or);

auto Language::Operations::Or::select_type(
    Tetrodotoxin::Language::Monograph&) const -> Core::Option<const Type&> {
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
    Memory::Allocator::Arena& domain)
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
