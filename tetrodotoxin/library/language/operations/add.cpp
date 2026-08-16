// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/language/operations/add.hpp"

#include "perimortem/core/math.hpp"

#include "tetrodotoxin/library/language/constants/real.hpp"
#include "tetrodotoxin/library/language/constants/signed.hpp"
#include "tetrodotoxin/library/language/constants/unsigned.hpp"
#include "tetrodotoxin/library/language/model/types/real.hpp"
#include "tetrodotoxin/library/language/model/types/signed.hpp"
#include "tetrodotoxin/library/language/model/types/unsigned.hpp"
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
  const Abstract& left_resolved = left.get_type().resolve();
  const Abstract& right_resolved = right.get_type().resolve();
  if (!left_resolved.is<Language::Model::Type>() ||
      &left_resolved != &right_resolved ||
      (!left_resolved.is<Language::Model::Types::Unsigned>() &&
       !left_resolved.is<Language::Model::Types::Signed>() &&
       !left_resolved.is<Language::Model::Types::Real>())) {
    return Invalid::get_invalid();
  }

  // Add fixes the result to the one identity already shared by both operands.
  // Arithmetic never becomes permission to retag or reinterpret a Constant.
  return left_resolved;
}

static auto signed_sum(
    const Tetrodotoxin::Library::Language::Model::Types::Signed& type,
    Signed_64 left,
    Signed_64 right,
    Signed_64& result) -> Bool {
  if (__builtin_add_overflow(left, right, &result)) {
    return False;
  }

  return Core::Math::is_representable(result, type.get_size());
}

static auto unsigned_sum(
    const Tetrodotoxin::Library::Language::Model::Types::Unsigned& type,
    Unsigned_64 left,
    Unsigned_64 right,
    Unsigned_64& result) -> Bool {
  if (__builtin_add_overflow(left, right, &result)) {
    return False;
  }

  return Core::Math::is_representable(result, type.get_size());
}

TTX_BINARY_PARSE(Add, AddOp);

TTX_BINARY_OP(Add);

auto Language::Operations::Add::select_type(const Ttx::Concept::Abstract&) const
    -> Core::Option<const Language::Model::Type&> {
  auto left = get_input(0);
  auto right = get_input(1);
  if (!left || !right) {
    return {};
  }

  return select_result_type(*left, *right).select<Language::Model::Type>();
}

auto Language::Operations::Add::evaluate_constants(
    Memory::Allocator::Arena& domain)
    -> Utility::Result<Core::Option<Constant&>, Expression::Error> {
  const Abstract& selected = get_type().resolve();
  auto authored_left = get_input(0);
  auto authored_right = get_input(1);
  auto left = get_folded_input(0);
  auto right = get_folded_input(1);
  if (!authored_left || !authored_right || !left || !right) {
    return Expression::Error(Expression::Error::Type::InvalidInput, *this);
  }

  // Linking already fixed one exact domain. The visitors prove the payload
  // contract for that domain and preserve the authored origin on mismatch.
  if (selected.is<Tetrodotoxin::Library::Language::Model::Types::Signed>()) {
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

    return selected.visit<
        Tetrodotoxin::Library::Language::Model::Types::Signed>(
        [&](const Tetrodotoxin::Library::Language::Model::Types::Signed& type)
            -> Utility::Result<Core::Option<Constant&>, Expression::Error> {
          Signed_64 value = 0;
          if (!signed_sum(
                  type, left_value->get_value(), right_value->get_value(),
                  value)) {
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

  if (selected.is<Tetrodotoxin::Library::Language::Model::Types::Unsigned>()) {
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

    return selected.visit<
        Tetrodotoxin::Library::Language::Model::Types::Unsigned>(
        [&](const Tetrodotoxin::Library::Language::Model::Types::Unsigned& type)
            -> Utility::Result<Core::Option<Constant&>, Expression::Error> {
          Unsigned_64 value = 0;
          if (!unsigned_sum(
                  type, left_value->get_value(), right_value->get_value(),
                  value)) {
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

  if (selected.is<Tetrodotoxin::Library::Language::Model::Types::Real>()) {
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

    return selected.visit<Tetrodotoxin::Library::Language::Model::Types::Real>(
        [&](const Tetrodotoxin::Library::Language::Model::Types::Real& type)
            -> Utility::Result<Core::Option<Constant&>, Expression::Error> {
          if (type.get_size() == sizeof(Real_32)) {
            Real_32 value = Real_32(left_value->get_value()) +
                            Real_32(right_value->get_value());
            return Constants::Real::create_synthetic(
                domain, type, Real_64(value));
          }

          if (type.get_size() == sizeof(Real_64)) {
            return Constants::Real::create_synthetic(
                domain, type,
                left_value->get_value() + right_value->get_value());
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
