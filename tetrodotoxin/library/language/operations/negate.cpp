// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/language/operations/negate.hpp"

#include "perimortem/core/math.hpp"

#include "tetrodotoxin/library/language/constants/real.hpp"
#include "tetrodotoxin/library/language/constants/signed.hpp"
#include "tetrodotoxin/library/language/parser/expression.hpp"
#include "ttx/concept/invalid.hpp"
#include "ttx/model/types/real.hpp"
#include "ttx/model/types/signed.hpp"

using namespace Perimortem;
using namespace Tetrodotoxin::Library;
using namespace Ttx::Concept;
using namespace Ttx::Lexical;
using namespace Ttx::Model;

static auto is_negatable_type(const Abstract& selected) -> Bool {
  return selected.visit<Ttx::Model::Types::Signed>(
      [](const Ttx::Model::Types::Signed& type) {
        return type.get_size() > 0 && type.get_size() <= sizeof(Signed_64)
                   ? True
                   : False;
      },
      [](const Abstract& selected) {
        return selected.visit<Ttx::Model::Types::Real>(
            [](const Ttx::Model::Types::Real& type) {
              return type.get_size() == sizeof(Real_32) ||
                             type.get_size() == sizeof(Real_64)
                         ? True
                         : False;
            },
            [](const Abstract&) { return False; });
      });
}

static auto select_result_type(const Language::Expression& operand)
    -> const Abstract& {
  const Abstract& selected = operand.get_type().resolve();
  if (!selected.is<Type>() || !is_negatable_type(selected)) {
    return Invalid::get_invalid();
  }

  return selected;
}

static auto signed_inverse(
    const Ttx::Model::Types::Signed& type,
    Signed_64 operand,
    Signed_64& result) -> Bool {
  if (__builtin_sub_overflow(Signed_64(0), operand, &result)) {
    return False;
  }

  return Core::Math::is_representable(result, type.get_size());
}

TTX_DIRECT_UNARY_PARSE(
    Negate,
    "Negate has a malformed operand."_view,
    "Use a complete Expression after unary `-`."_view,
    "Negate requires an authored operand Anchor."_view);

TTX_UNARY_OP(Negate);

auto Language::Operations::Negate::select_type(Materializations&) const
    -> Core::Option<const Type&> {
  auto operand = get_input(0);
  if (!operand) {
    return {};
  }

  return select_result_type(*operand).select<Type>();
}

auto Language::Operations::Negate::evaluate_constants(
    Memory::Allocator::Arena& domain,
    Materializations&)
    -> Utility::Result<Core::Option<Constant&>, Expression::Error> {
  const Abstract& selected = get_type().resolve();
  auto authored_operand = get_input(0);
  auto operand = get_folded_input(0);
  if (!authored_operand || !operand) {
    return Expression::Error(Expression::Error::Type::InvalidInput, *this);
  }

  // Linking fixes the exact result Type before folding. The visitors prove
  // only the Constant payload needed to calculate its inverse.
  if (selected.is<Ttx::Model::Types::Signed>()) {
    auto value = operand->select<Constants::Signed>();
    if (!value) {
      return Expression::Error(
          Expression::Error::Type::InvalidConstant, *authored_operand);
    }

    return selected.visit<Ttx::Model::Types::Signed>(
        [&](const Ttx::Model::Types::Signed& type)
            -> Utility::Result<Core::Option<Constant&>, Expression::Error> {
          Signed_64 inverse = 0;
          if (!signed_inverse(type, value->get_value(), inverse)) {
            return Expression::Error(
                Expression::Error::Type::ArithmeticOverflow, *this);
          }

          return Constants::Signed::create_synthetic(domain, type, inverse);
        },
        [&](const Abstract&)
            -> Utility::Result<Core::Option<Constant&>, Expression::Error> {
          return Expression::Error(
              Expression::Error::Type::InvalidOperationType, *this);
        });
  }

  if (selected.is<Ttx::Model::Types::Real>()) {
    auto value = operand->select<Constants::Real>();
    if (!value) {
      return Expression::Error(
          Expression::Error::Type::InvalidConstant, *authored_operand);
    }

    return selected.visit<Ttx::Model::Types::Real>(
        [&](const Ttx::Model::Types::Real& type)
            -> Utility::Result<Core::Option<Constant&>, Expression::Error> {
          if (type.get_size() == sizeof(Real_32)) {
            Real_32 inverse = -Real_32(value->get_value());
            return Constants::Real::create_synthetic(
                domain, type, Real_64(inverse));
          }

          if (type.get_size() == sizeof(Real_64)) {
            return Constants::Real::create_synthetic(
                domain, type, -value->get_value());
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
