// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/language/operations/greater.hpp"

#include "perimortem/core/static/vector.hpp"

#include "tetrodotoxin/library/dialect.hpp"
#include "tetrodotoxin/library/language/constants/false.hpp"
#include "tetrodotoxin/library/language/constants/real.hpp"
#include "tetrodotoxin/library/language/constants/signed.hpp"
#include "tetrodotoxin/library/language/constants/true.hpp"
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

template <typename selected_type>
static auto select_constant(const Language::Expression& expression)
    -> Utility::Option<const selected_type&> {
  return expression.visit<selected_type>(
      [](const selected_type& selected)
          -> Utility::Option<const selected_type&> { return selected; },
      [](const Abstract&) -> Utility::Option<const selected_type&> {
        return {};
      });
}

static auto is_numeric_type(const Abstract& selected) -> Bool {
  return selected.visit<Ttx::Model::Types::Signed>(
      [](const Ttx::Model::Types::Signed& type) {
        return type.get_size() > 0 && type.get_size() <= sizeof(Signed_64)
                   ? True
                   : False;
      },
      [](const Abstract& selected) {
        return selected.visit<Ttx::Model::Types::Unsigned>(
            [](const Ttx::Model::Types::Unsigned& type) {
              return type.get_size() > 0 &&
                             type.get_size() <= sizeof(Unsigned_64)
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
      });
}

static auto select_operand_type(
    const Language::Expression& left,
    const Language::Expression& right) -> const Abstract& {
  const Abstract& left_resolved = left.get_type().resolve();
  const Abstract& right_resolved = right.get_type().resolve();
  if (!left_resolved.is<Type>() || &left_resolved != &right_resolved ||
      !is_numeric_type(left_resolved)) {
    return Invalid::get_invalid();
  }

  // Runtime values and Constants use the same exact operand Type. Conversion
  // belongs to a receiving typed owner before Greater construction.
  return left_resolved;
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

auto Language::Operations::Greater::parse(
    Memory::Allocator::Arena& domain,
    Materializations& materializations,
    Cursor& cursor,
    const Abstract& source_context,
    Expression& left) -> Utility::Option<Expression&> {
  auto transaction = cursor.branch();
  Token opening = transaction.consume();
  auto right = Language::Parser::Expression::parse_operand(
      domain, materializations, transaction, source_context,
      Code::Type::GreaterOp);
  Span span(opening, transaction.peek(-1));
  if (!right) {
    transaction.create_expression_error(
        span, "Greater has a malformed right operand."_view,
        "Use a complete scalar Expression after `>`."_view);
    return {};
  }

  const auto& left_anchor = left.get_anchor();
  const auto& right_anchor = right->get_anchor();
  if (!left_anchor || !right_anchor) {
    transaction.create_expression_error(
        span, "Greater requires authored operand Anchors."_view);
    return {};
  }

  auto anchor = Anchor::create(
      opening, left_anchor->get_span(), right_anchor->get_span());
  auto& greater =
      create_authored(domain, materializations, left, *right, anchor);
  cursor.join(transaction);
  return greater;
}

auto Language::Operations::Greater::create_authored(
    Memory::Allocator::Arena& domain,
    Materializations& materializations,
    Expression& left,
    Expression& right,
    Anchor anchor) -> Greater& {
  return Expression::create_authored<Greater>(
      domain, anchor, [&](auto source) -> Greater {
        return Greater(domain, materializations, left, right, source);
      });
}

auto Language::Operations::Greater::create_synthetic(
    Memory::Allocator::Arena& domain,
    Materializations& materializations,
    Expression& left,
    Expression& right) -> Greater& {
  return Expression::create_synthetic<Greater>(
      domain, [&](auto source) -> Greater {
        return Greater(domain, materializations, left, right, source);
      });
}

Language::Operations::Greater::Greater(
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

auto Language::Operations::Greater::get_documentation() const
    -> const Documentation& {
  return Documentation::get_empty();
}

auto Language::Operations::Greater::select_type(Materializations&) const
    -> Utility::Option<const Type&> {
  auto left = get_input(0);
  auto right = get_input(1);
  if (!left || !right ||
      !select_operand_type(*left, *right).resolve().is<Type>()) {
    return {};
  }

  return Dialect::get_bool();
}

auto Language::Operations::Greater::evaluate_constants(
    Memory::Allocator::Arena& domain,
    Materializations&)
    -> Utility::Result<Utility::Option<Constant&>, Expression::Error> {
  auto authored_left = get_input(0);
  auto authored_right = get_input(1);
  auto left = get_folded_input(0);
  auto right = get_folded_input(1);
  if (!authored_left || !authored_right || !left || !right) {
    return Expression::Error(Expression::Error::Type::InvalidInput, *this);
  }

  const Abstract& selected = left->get_type().resolve();

  // The operand domain was established before folding. These visitors prove
  // matching Constant payloads while every result uses canonical Bool.
  if (selected.is<Ttx::Model::Types::Signed>()) {
    auto left_value = select_constant<Constants::Signed>(*left);
    auto right_value = select_constant<Constants::Signed>(*right);
    if (!left_value) {
      return Expression::Error(
          Expression::Error::Type::InvalidConstant, *authored_left);
    }

    if (!right_value) {
      return Expression::Error(
          Expression::Error::Type::InvalidConstant, *authored_right);
    }

    return make_result(
        domain, left_value->get_value() > right_value->get_value());
  }

  if (selected.is<Ttx::Model::Types::Unsigned>()) {
    auto left_value = select_constant<Constants::Unsigned>(*left);
    auto right_value = select_constant<Constants::Unsigned>(*right);
    if (!left_value) {
      return Expression::Error(
          Expression::Error::Type::InvalidConstant, *authored_left);
    }

    if (!right_value) {
      return Expression::Error(
          Expression::Error::Type::InvalidConstant, *authored_right);
    }

    return make_result(
        domain, left_value->get_value() > right_value->get_value());
  }

  if (selected.is<Ttx::Model::Types::Real>()) {
    auto left_value = select_constant<Constants::Real>(*left);
    auto right_value = select_constant<Constants::Real>(*right);
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
            -> Utility::Result<Utility::Option<Constant&>, Expression::Error> {
          if (type.get_size() == sizeof(Real_32)) {
            return make_result(
                domain, Real_32(left_value->get_value()) >
                            Real_32(right_value->get_value()));
          }

          if (type.get_size() == sizeof(Real_64)) {
            return make_result(
                domain, left_value->get_value() > right_value->get_value());
          }

          return Expression::Error(
              Expression::Error::Type::InvalidOperationType, *this);
        },
        [&](const Abstract&)
            -> Utility::Result<Utility::Option<Constant&>, Expression::Error> {
          return Expression::Error(
              Expression::Error::Type::InvalidOperationType, *this);
        });
  }

  return Expression::Error(
      Expression::Error::Type::InvalidOperationType, *this);
}
