// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/language/operations/equal.hpp"

#include "perimortem/core/static/vector.hpp"

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

static auto select_constant(Language::Expression& expression)
    -> Core::Option<Language::Constant&> {
  return expression.visit<Language::Constant>(
      [](Language::Constant& selected) -> Core::Option<Language::Constant&> {
        return selected;
      },
      [](Abstract&) -> Core::Option<Language::Constant&> { return {}; });
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

auto Language::Operations::Equal::parse(
    Memory::Allocator::Arena& domain,
    Materializations& materializations,
    Cursor& cursor,
    const Abstract& source_context,
    Expression& left) -> Core::Option<Expression&> {
  auto transaction = cursor.branch();
  Token opening = transaction.consume();
  auto right = Language::Parser::Expression::parse_operand(
      domain, materializations, transaction, source_context, Code::Type::CmpOp);
  Span span(opening, transaction.peek(-1));
  if (!right) {
    transaction.create_expression_error(
        span, "Equal has a malformed right operand."_view,
        "Use a complete scalar or Bytes Expression after `==`."_view);
    return {};
  }

  const auto& left_anchor = left.get_anchor();
  const auto& right_anchor = right->get_anchor();
  if (!left_anchor || !right_anchor) {
    transaction.create_expression_error(
        span, "Equal requires authored operand Anchors."_view);
    return {};
  }

  auto anchor = Anchor::create(
      opening, left_anchor->get_span(), right_anchor->get_span());
  auto& equal = create_authored(domain, materializations, left, *right, anchor);
  cursor.join(transaction);
  return equal;
}

auto Language::Operations::Equal::create_authored(
    Memory::Allocator::Arena& domain,
    Materializations& materializations,
    Expression& left,
    Expression& right,
    Anchor anchor) -> Equal& {
  return Expression::create_authored<Equal>(
      domain, anchor, [&](auto source) -> Equal {
        return Equal(domain, materializations, left, right, source);
      });
}

auto Language::Operations::Equal::create_synthetic(
    Memory::Allocator::Arena& domain,
    Materializations& materializations,
    Expression& left,
    Expression& right) -> Equal& {
  return Expression::create_synthetic<Equal>(domain, [&](auto source) -> Equal {
    return Equal(domain, materializations, left, right, source);
  });
}

Language::Operations::Equal::Equal(
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

auto Language::Operations::Equal::get_documentation() const
    -> const Documentation& {
  return Documentation::get_empty();
}

auto Language::Operations::Equal::select_type(Materializations&) const
    -> Core::Option<const Type&> {
  auto left = get_input(0);
  auto right = get_input(1);
  if (!left || !right ||
      !select_operand_type(*left, *right).resolve().is<Type>()) {
    return {};
  }

  return Dialect::get_bool();
}

auto Language::Operations::Equal::evaluate_constants(
    Memory::Allocator::Arena& domain,
    Materializations&)
    -> Utility::Result<Core::Option<Constant&>, Expression::Error> {
  auto authored_left = get_input(0);
  auto authored_right = get_input(1);
  auto left = get_folded_input(0);
  auto right = get_folded_input(1);
  if (!authored_left || !authored_right || !left || !right) {
    return Expression::Error(Expression::Error::Type::InvalidInput, *this);
  }

  const Abstract& selected = left->get_type().resolve();

  // Constant owns each payload comparison and exact Type identity. Equal only
  // proves that the completed inputs still belong to the selected domain.
  auto left_value = select_constant(*left);
  auto right_value = select_constant(*right);
  if (!left_value || !matches_domain(*left, selected)) {
    return Expression::Error(
        Expression::Error::Type::InvalidConstant, *authored_left);
  }

  if (!right_value || !matches_domain(*right, selected)) {
    return Expression::Error(
        Expression::Error::Type::InvalidConstant, *authored_right);
  }

  return make_result(domain, *left_value == *right_value);
}
