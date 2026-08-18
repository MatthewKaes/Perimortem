// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/language/operations/multiply.hpp"

#include "perimortem/core/math.hpp"

#include "tetrodotoxin/library/language/constants/real.hpp"
#include "tetrodotoxin/library/language/constants/signed.hpp"
#include "tetrodotoxin/library/language/constants/unsigned.hpp"
#include "tetrodotoxin/library/language/model/types/real.hpp"
#include "tetrodotoxin/library/language/model/types/signed.hpp"
#include "tetrodotoxin/library/language/model/types/unsigned.hpp"
#include "tetrodotoxin/library/language/parser/expression.hpp"
#include "tetrodotoxin/library/llvm/builder.hpp"
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

  // Literal Constants keep their declared Type here. Conversion belongs to a
  // receiving typed owner, so runtime values and completed values follow the
  // same exact identity rule.
  return left_resolved;
}

static auto signed_product(
    const Tetrodotoxin::Library::Language::Model::Types::Signed& type,
    Signed_64 left,
    Signed_64 right,
    Signed_64& result) -> Bool {
  if (__builtin_mul_overflow(left, right, &result)) {
    return False;
  }

  return Core::Math::is_representable(result, type.get_size());
}

static auto unsigned_product(
    const Tetrodotoxin::Library::Language::Model::Types::Unsigned& type,
    Unsigned_64 left,
    Unsigned_64 right,
    Unsigned_64& result) -> Bool {
  if (__builtin_mul_overflow(left, right, &result)) {
    return False;
  }

  return Core::Math::is_representable(result, type.get_size());
}

TTX_BINARY_PARSE(Multiply, MulOp);

TTX_BINARY_OP(Multiply);

auto Language::Operations::Multiply::lower(Llvm::Builder& body) const -> Bool {
  auto folded = lower_folded(body);
  if (folded) {
    return *folded;
  }

  auto inputs = get_inputs();
  const Expression& left = inputs.get_data()[0].get();
  const Expression& right = inputs.get_data()[1].get();
  auto carrier = get_type().resolve().select<Ttx::Model::Type>();

  if (!carrier) {
    return False;
  }

  Bool lowered = lower_inputs(body);
  if (!lowered) {
    return False;
  }

  return body.multiply(*carrier, *this, left, right);
}

auto Language::Operations::Multiply::select_type(const Ttx::Concept::Abstract&)
    const -> Core::Option<const Language::Model::Type&> {
  auto inputs = get_inputs();
  const Expression& left = inputs.get_data()[0].get();
  const Expression& right = inputs.get_data()[1].get();
  return select_result_type(left, right).select<Language::Model::Type>();
}

auto Language::Operations::Multiply::evaluate_constants(
    Memory::Allocator::Arena& domain)
    -> Utility::Result<Core::Option<Constant&>, Expression::Error> {
  const Abstract& selected = get_type().resolve();
  auto inputs = get_inputs();
  Expression& authored_left = inputs.get_data()[0].get();
  Expression& authored_right = inputs.get_data()[1].get();
  auto left = get_folded_input(0);
  auto right = get_folded_input(1);
  if (!left || !right) {
    return Expression::Error(Expression::Error::Type::InvalidInput, *this);
  }

  // Type legality has already fixed one exact domain before folding begins.
  // These visitors prove that completed payloads still implement that domain
  // instead of treating an error category as permission to cast them.
  if (selected.is<Tetrodotoxin::Library::Language::Model::Types::Signed>()) {
    auto left_value = left->select<Constants::Signed>();
    auto right_value = right->select<Constants::Signed>();
    if (!left_value) {
      return Expression::Error(
          Expression::Error::Type::InvalidConstant, authored_left);
    }

    if (!right_value) {
      return Expression::Error(
          Expression::Error::Type::InvalidConstant, authored_right);
    }

    return selected.visit<
        Tetrodotoxin::Library::Language::Model::Types::Signed>(
        [&](const Tetrodotoxin::Library::Language::Model::Types::Signed& type)
            -> Utility::Result<Core::Option<Constant&>, Expression::Error> {
          Signed_64 value = 0;
          if (!signed_product(
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
          Expression::Error::Type::InvalidConstant, authored_left);
    }

    if (!right_value) {
      return Expression::Error(
          Expression::Error::Type::InvalidConstant, authored_right);
    }

    return selected.visit<
        Tetrodotoxin::Library::Language::Model::Types::Unsigned>(
        [&](const Tetrodotoxin::Library::Language::Model::Types::Unsigned& type)
            -> Utility::Result<Core::Option<Constant&>, Expression::Error> {
          Unsigned_64 value = 0;
          if (!unsigned_product(
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
          Expression::Error::Type::InvalidConstant, authored_left);
    }

    if (!right_value) {
      return Expression::Error(
          Expression::Error::Type::InvalidConstant, authored_right);
    }

    return selected.visit<Tetrodotoxin::Library::Language::Model::Types::Real>(
        [&](const Tetrodotoxin::Library::Language::Model::Types::Real& type)
            -> Utility::Result<Core::Option<Constant&>, Expression::Error> {
          if (type.get_size() == sizeof(Real_32)) {
            Real_32 value = Real_32(left_value->get_value()) *
                            Real_32(right_value->get_value());
            return Constants::Real::create_synthetic(
                domain, type, Real_64(value));
          }

          if (type.get_size() == sizeof(Real_64)) {
            return Constants::Real::create_synthetic(
                domain, type,
                left_value->get_value() * right_value->get_value());
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
