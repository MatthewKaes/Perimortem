// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/language/operations/subtract.hpp"

#include "perimortem/core/static/vector.hpp"
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

static auto is_numeric_type(const Abstract& selected) -> Bool {
  return selected.is<Ttx::Model::Types::Unsigned>() ||
         selected.is<Ttx::Model::Types::Signed>() ||
         selected.is<Ttx::Model::Types::Real>();
}

static auto select_result_type(
    const Language::Expression& left,
    const Language::Expression& right) -> const Abstract& {
  const Abstract& left_resolved = left.get_type().resolve();
  const Abstract& right_resolved = right.get_type().resolve();
  if (!left_resolved.is<Type>() || &left_resolved != &right_resolved ||
      !is_numeric_type(left_resolved)) {
    return Invalid::get_invalid();
  }

  // Subtract never retags a Constant to make the pair legal. Both retained
  // inputs expose this same Type before arithmetic begins.
  return left_resolved;
}

static auto signed_difference(
    const Ttx::Model::Types::Signed& type,
    Signed_64 left,
    Signed_64 right,
    Signed_64& result) -> Bool {
  if (__builtin_sub_overflow(left, right, &result)) {
    return False;
  }

  return Core::Math::is_representable(result, type.get_size());
}

static auto unsigned_difference(
    const Ttx::Model::Types::Unsigned& type,
    Unsigned_64 left,
    Unsigned_64 right,
    Unsigned_64& result) -> Bool {
  if (__builtin_sub_overflow(left, right, &result)) {
    return False;
  }

  return Core::Math::is_representable(result, type.get_size());
}

auto Language::Operations::Subtract::parse(
    Memory::Allocator::Arena& domain,
    Materializations& materializations,
    Cursor& cursor,
    const Abstract& source_context,
    Expression& left) -> Core::Option<Expression&> {
  Token opening = cursor.consume();
  auto right = Language::Parser::Expression::parse_operand(
      domain, materializations, cursor, source_context, Code::Type::SubOp);
  Span span(opening, cursor.peek(-1));
  if (!right) {
    cursor.create_expression_error(
        span, "Subtract has a malformed right operand."_view,
        "Use a complete scalar Expression after binary `-`."_view);
    return {};
  }

  const auto& left_anchor = left.get_anchor();
  const auto& right_anchor = right->get_anchor();
  if (!left_anchor || !right_anchor) {
    cursor.create_expression_error(
        span, "Subtract requires authored operand Anchors."_view);
    return {};
  }

  auto anchor = Anchor::create(
      opening, left_anchor->get_span(), right_anchor->get_span());
  return create_authored(domain, materializations, left, *right, anchor);
}

auto Language::Operations::Subtract::create_authored(
    Memory::Allocator::Arena& domain,
    Materializations& materializations,
    Expression& left,
    Expression& right,
    Anchor anchor) -> Subtract& {
  return Expression::create_authored<Subtract>(
      domain, anchor, [&](auto source) -> Subtract {
        return Subtract(domain, materializations, left, right, source);
      });
}

auto Language::Operations::Subtract::create_synthetic(
    Memory::Allocator::Arena& domain,
    Materializations& materializations,
    Expression& left,
    Expression& right) -> Subtract& {
  return Expression::create_synthetic<Subtract>(
      domain, [&](auto source) -> Subtract {
        return Subtract(domain, materializations, left, right, source);
      });
}

Language::Operations::Subtract::Subtract(
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

auto Language::Operations::Subtract::get_documentation() const
    -> const Documentation& {
  return Documentation::get_empty();
}

auto Language::Operations::Subtract::select_type(Materializations&) const
    -> Core::Option<const Type&> {
  auto left = get_input(0);
  auto right = get_input(1);
  if (!left || !right) {
    return {};
  }

  return select_result_type(*left, *right).select<Type>();
}

auto Language::Operations::Subtract::evaluate_constants(
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

  // Type legality fixes one exact domain before folding. Visitor proof here is
  // only the Constant payload contract needed to perform that operation.
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
          Signed_64 value = 0;
          if (!signed_difference(
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
          Unsigned_64 value = 0;
          if (!unsigned_difference(
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
            Real_32 value = Real_32(left_value->get_value()) -
                            Real_32(right_value->get_value());
            return Constants::Real::create_synthetic(
                domain, type, Real_64(value));
          }

          if (type.get_size() == sizeof(Real_64)) {
            return Constants::Real::create_synthetic(
                domain, type,
                left_value->get_value() - right_value->get_value());
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
