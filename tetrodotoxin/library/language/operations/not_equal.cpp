// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/language/operations/not_equal.hpp"

#include "tetrodotoxin/library/dialect.hpp"
#include "tetrodotoxin/library/language/constants/bytes.hpp"
#include "tetrodotoxin/library/language/constants/false.hpp"
#include "tetrodotoxin/library/language/constants/flag.hpp"
#include "tetrodotoxin/library/language/constants/real.hpp"
#include "tetrodotoxin/library/language/constants/signed.hpp"
#include "tetrodotoxin/library/language/constants/true.hpp"
#include "tetrodotoxin/library/language/constants/unsigned.hpp"
#include "tetrodotoxin/library/language/parser/expression.hpp"
#include "ttx/concept/invalid.hpp"
#include "ttx/model/types/flag.hpp"
#include "ttx/model/types/real.hpp"
#include "ttx/model/types/signed.hpp"
#include "ttx/model/types/unsigned.hpp"

using namespace Perimortem;
using namespace Tetrodotoxin::Library;
using namespace Ttx::Concept;
using namespace Ttx::Lexical;
using namespace Ttx::Model;

static auto select_operand_type(
    const Language::Expression& left,
    const Language::Expression& right) -> const Abstract& {
  const Abstract& left_resolved = left.get_type().resolve();
  const Abstract& right_resolved = right.get_type().resolve();
  if (!left_resolved.is<Type>() || &left_resolved != &right_resolved) {
    return Invalid::get_invalid();
  }

  if (left_resolved.is<Ttx::Model::Types::Value>() ||
      (left.is<Language::Constants::Bytes>() &&
       right.is<Language::Constants::Bytes>())) {
    return left_resolved;
  }

  // Bytes is a complete Constant payload domain rather than a universal Type
  // category. Admitting both values here keeps that ownership distinction.
  return Invalid::get_invalid();
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

static auto matches_domain(
    const Language::Expression& expression,
    const Abstract& selected) -> Bool {
  if (selected.is<Ttx::Model::Types::Signed>()) {
    return expression.is<Language::Constants::Signed>();
  }

  if (selected.is<Ttx::Model::Types::Unsigned>()) {
    return expression.is<Language::Constants::Unsigned>();
  }

  if (selected.is<Ttx::Model::Types::Real>()) {
    return expression.is<Language::Constants::Real>();
  }

  if (selected.is<Ttx::Model::Types::Flag>()) {
    return expression.is<Language::Constants::Flag>();
  }

  return expression.is<Language::Constants::Bytes>();
}

TTX_TRANSACTIONAL_BINARY_PARSE(
    NotEqual,
    NotEqOp,
    "NotEqual has a malformed right operand."_view,
    "Use a complete scalar or Bytes Expression after `!=`."_view);

TTX_BINARY_OP(NotEqual);

auto Language::Operations::NotEqual::select_type(
    Tetrodotoxin::Language::Monograph&) const -> Core::Option<const Type&> {
  auto left = get_input(0);
  auto right = get_input(1);
  if (!left || !right ||
      !select_operand_type(*left, *right).resolve().is<Type>()) {
    return {};
  }

  return Dialect::get_bool();
}

auto Language::Operations::NotEqual::evaluate_constants(
    Memory::Allocator::Arena& domain)
    -> Utility::Result<Core::Option<Constant&>, Expression::Error> {
  auto authored_left = get_input(0);
  auto authored_right = get_input(1);
  auto left = get_folded_input(0);
  auto right = get_folded_input(1);
  if (!authored_left || !authored_right || !left || !right) {
    return Expression::Error(Expression::Error::Type::InvalidInput, *this);
  }

  const Abstract& selected = left->get_type().resolve();

  // Constant owns each payload comparison and exact Type identity. NotEqual
  // only proves that completed inputs still belong to the selected domain.
  auto left_value = left->select<Constant>();
  auto right_value = right->select<Constant>();
  if (!left_value || !matches_domain(*left, selected)) {
    return Expression::Error(
        Expression::Error::Type::InvalidConstant, *authored_left);
  }

  if (!right_value || !matches_domain(*right, selected)) {
    return Expression::Error(
        Expression::Error::Type::InvalidConstant, *authored_right);
  }

  return make_result(domain, *left_value != *right_value);
}
