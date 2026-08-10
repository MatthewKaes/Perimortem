// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/language/operations/divide.hpp"

#include "perimortem/core/math.hpp"

#include "tetrodotoxin/library/language/constants/real.hpp"
#include "tetrodotoxin/library/language/constants/signed.hpp"
#include "tetrodotoxin/library/language/constants/unsigned.hpp"
#include "tetrodotoxin/library/language/parser/expression.hpp"
#include "ttx/concept/invalid.hpp"
#include "ttx/model/types/real.hpp"
#include "ttx/model/types/signed.hpp"
#include "ttx/model/types/unsigned.hpp"

using namespace Perimortem;
using namespace Tetrodotoxin::Library;
using namespace Ttx::Concept;
using namespace Ttx::Lexical;
using namespace Ttx::Model;

static auto select_result_type(
    const Language::Expression& left,
    const Language::Expression& right) -> const Abstract& {
  const Abstract& left_resolved = left.get_type().resolve();
  const Abstract& right_resolved = right.get_type().resolve();
  if (!left_resolved.is<Type>() || &left_resolved != &right_resolved ||
      (!left_resolved.is<Types::Unsigned>() &&
       !left_resolved.is<Types::Signed>() &&
       !left_resolved.is<Types::Real>())) {
    return Invalid::get_invalid();
  }

  // Divide keeps both authored Types exact. A receiving typed owner performs
  // any conversion before construction so runtime and Constant inputs agree.
  return left_resolved;
}

TTX_TRANSACTIONAL_BINARY_PARSE(
    Divide,
    DivOp,
    "Divide has a malformed right operand."_view,
    "Use a complete scalar Expression after `/`."_view,
    "Divide requires authored operand Anchors."_view);

TTX_BINARY_OP(Divide);

auto Language::Operations::Divide::select_type(Materializations&) const
    -> Core::Option<const Type&> {
  auto left = get_input(0);
  auto right = get_input(1);
  if (!left || !right) {
    return {};
  }

  return select_result_type(*left, *right).select<Type>();
}

auto Language::Operations::Divide::evaluate_constants(
    Memory::Allocator::Arena& domain,
    Materializations&)
    -> Utility::Result<Core::Option<Constant&>, Expression::Error> {
  const Abstract& selected = get_type().resolve();
  auto authored_left = get_input(0);
  auto authored_right = get_input(1);
  auto left = get_folded_input(0);
  auto right = get_folded_input(1);
  if (!authored_left || !authored_right || !left || !right) {
    return Expression::Error(Expression::Error::Type::InvalidInput, *this);
  }

  // Type selection fixes the payload domain before folding. Integer guards
  // run before host arithmetic so zero and the signed endpoint stay failures.
  if (selected.is<Ttx::Model::Types::Signed>()) {
    auto left_value = left->select<Constants::Signed>();
    auto right_value = right->select<Constants::Signed>();
    if (!left_value) {
      return Expression::Error(
          Expression::Error::Type::InvalidConstant, *authored_left);
    }

    if (!right_value) {
      return Expression::Error(
          Expression::Error::Type::InvalidConstant, *authored_right);
    }

    return selected.visit<Ttx::Model::Types::Signed>(
        [&](const Ttx::Model::Types::Signed& type)
            -> Utility::Result<Core::Option<Constant&>, Expression::Error> {
          Signed_64 divisor = right_value->get_value();
          if (divisor == 0) {
            return Expression::Error(
                Expression::Error::Type::DivisionByZero, *this);
          }

          Signed_64 value = 0;
          if (divisor == -1) {
            Bool overflow = __builtin_sub_overflow(
                Signed_64(0), left_value->get_value(), &value);
            if (overflow ||
                !Core::Math::is_representable(value, type.get_size())) {
              return Expression::Error(
                  Expression::Error::Type::ArithmeticOverflow, *this);
            }
          } else {
            value = left_value->get_value() / divisor;
          }

          if (!Core::Math::is_representable(value, type.get_size())) {
            return Expression::Error(
                Expression::Error::Type::ArithmeticOverflow, *this);
          }

          return Constants::Signed::create_synthetic(domain, type, value);
        },
        [&](const Abstract&)
            -> Utility::Result<Core::Option<Constant&>, Expression::Error> {
          return Expression::Error(
              Expression::Error::Type::InvalidOperationType, *this);
        });
  }

  if (selected.is<Ttx::Model::Types::Unsigned>()) {
    auto left_value = left->select<Constants::Unsigned>();
    auto right_value = right->select<Constants::Unsigned>();
    if (!left_value) {
      return Expression::Error(
          Expression::Error::Type::InvalidConstant, *authored_left);
    }

    if (!right_value) {
      return Expression::Error(
          Expression::Error::Type::InvalidConstant, *authored_right);
    }

    return selected.visit<Ttx::Model::Types::Unsigned>(
        [&](const Ttx::Model::Types::Unsigned& type)
            -> Utility::Result<Core::Option<Constant&>, Expression::Error> {
          Unsigned_64 divisor = right_value->get_value();
          if (divisor == 0) {
            return Expression::Error(
                Expression::Error::Type::DivisionByZero, *this);
          }

          Unsigned_64 value = left_value->get_value() / divisor;
          if (!Core::Math::is_representable(value, type.get_size())) {
            return Expression::Error(
                Expression::Error::Type::ArithmeticOverflow, *this);
          }

          return Constants::Unsigned::create_synthetic(domain, type, value);
        },
        [&](const Abstract&)
            -> Utility::Result<Core::Option<Constant&>, Expression::Error> {
          return Expression::Error(
              Expression::Error::Type::InvalidOperationType, *this);
        });
  }

  if (selected.is<Ttx::Model::Types::Real>()) {
    auto left_value = left->select<Constants::Real>();
    auto right_value = right->select<Constants::Real>();
    if (!left_value) {
      return Expression::Error(
          Expression::Error::Type::InvalidConstant, *authored_left);
    }

    if (!right_value) {
      return Expression::Error(
          Expression::Error::Type::InvalidConstant, *authored_right);
    }

    return selected.visit<Ttx::Model::Types::Real>(
        [&](const Ttx::Model::Types::Real& type)
            -> Utility::Result<Core::Option<Constant&>, Expression::Error> {
          if (type.get_size() == sizeof(Real_32)) {
            Real_32 value = Real_32(left_value->get_value()) /
                            Real_32(right_value->get_value());
            return Constants::Real::create_synthetic(
                domain, type, Real_64(value));
          }

          if (type.get_size() == sizeof(Real_64)) {
            return Constants::Real::create_synthetic(
                domain, type,
                left_value->get_value() / right_value->get_value());
          }

          return Expression::Error(
              Expression::Error::Type::InvalidOperationType, *this);
        },
        [&](const Abstract&)
            -> Utility::Result<Core::Option<Constant&>, Expression::Error> {
          return Expression::Error(
              Expression::Error::Type::InvalidOperationType, *this);
        });
  }

  return Expression::Error(
      Expression::Error::Type::InvalidOperationType, *this);
}
