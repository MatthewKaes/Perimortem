// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/language/operations/modulo.hpp"

#include "perimortem/core/static/vector.hpp"
#include "perimortem/core/math.hpp"

#include "tetrodotoxin/library/language/constants/signed.hpp"
#include "tetrodotoxin/library/language/constants/unsigned.hpp"
#include "tetrodotoxin/library/language/parser/expression.hpp"
#include "ttx/concept/invalid.hpp"
#include "ttx/model/types/signed.hpp"
#include "ttx/model/types/unsigned.hpp"

using namespace Perimortem;
using namespace Tetrodotoxin::Library;
using namespace Ttx::Concept;
using namespace Ttx::Lexical;
using namespace Ttx::Model;

template <typename selected_type>
static auto select_constant(const Language::Expression& expression)
    -> Core::Option<const selected_type&> {
  return expression.visit<selected_type>(
      [](const selected_type& selected) -> Core::Option<const selected_type&> {
        return selected;
      },
      [](const Abstract&) -> Core::Option<const selected_type&> { return {}; });
}

static auto is_integer_type(const Abstract& selected) -> Bool {
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
            [](const Abstract&) { return False; });
      });
}

static auto select_result_type(
    const Language::Expression& left,
    const Language::Expression& right) -> const Abstract& {
  const Abstract& left_resolved = left.get_type().resolve();
  const Abstract& right_resolved = right.get_type().resolve();
  if (!left_resolved.is<Type>() || &left_resolved != &right_resolved ||
      !is_integer_type(left_resolved)) {
    return Invalid::get_invalid();
  }

  // Modulo keeps the authored integer Type exact. A receiving typed owner
  // performs any conversion before construction so every input follows it.
  return left_resolved;
}

auto Language::Operations::Modulo::parse(
    Memory::Allocator::Arena& domain,
    Materializations& materializations,
    Cursor& cursor,
    const Abstract& source_context,
    Expression& left) -> Core::Option<Expression&> {
  auto transaction = cursor.branch();
  Token opening = transaction.consume();
  auto right = Language::Parser::Expression::parse_operand(
      domain, materializations, transaction, source_context, Code::Type::ModOp);
  Span span(opening, transaction.peek(-1));
  if (!right) {
    transaction.create_expression_error(
        span, "Modulo has a malformed right operand."_view,
        "Use a complete integer Expression after `%`."_view);
    return {};
  }

  const auto& left_anchor = left.get_anchor();
  const auto& right_anchor = right->get_anchor();
  if (!left_anchor || !right_anchor) {
    transaction.create_expression_error(
        span, "Modulo requires authored operand Anchors."_view);
    return {};
  }

  auto anchor = Anchor::create(
      opening, left_anchor->get_span(), right_anchor->get_span());
  auto& modulo =
      create_authored(domain, materializations, left, *right, anchor);
  cursor.join(transaction);
  return modulo;
}

auto Language::Operations::Modulo::create_authored(
    Memory::Allocator::Arena& domain,
    Materializations& materializations,
    Expression& left,
    Expression& right,
    Anchor anchor) -> Modulo& {
  return Expression::create_authored<Modulo>(
      domain, anchor, [&](auto source) -> Modulo {
        return Modulo(domain, materializations, left, right, source);
      });
}

auto Language::Operations::Modulo::create_synthetic(
    Memory::Allocator::Arena& domain,
    Materializations& materializations,
    Expression& left,
    Expression& right) -> Modulo& {
  return Expression::create_synthetic<Modulo>(
      domain, [&](auto source) -> Modulo {
        return Modulo(domain, materializations, left, right, source);
      });
}

Language::Operations::Modulo::Modulo(
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

auto Language::Operations::Modulo::get_documentation() const
    -> const Documentation& {
  return Documentation::get_empty();
}

auto Language::Operations::Modulo::select_type(Materializations&) const
    -> Core::Option<const Type&> {
  auto left = get_input(0);
  auto right = get_input(1);
  if (!left || !right) {
    return {};
  }

  return select_result_type(*left, *right)
      .visit<Type>(
          [](const Type& type) -> Core::Option<const Type&> { return type; },
          [](const Abstract&) -> Core::Option<const Type&> { return {}; });
}

auto Language::Operations::Modulo::evaluate_constants(
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

  // The selected integer domain is fixed before folding. Guards run before
  // host remainder so zero and the signed endpoint stay durable failures.
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
            Signed_64 negated = 0;
            Bool overflow = __builtin_sub_overflow(
                Signed_64(0), left_value->get_value(), &negated);
            if (overflow ||
                !Core::Math::is_representable(negated, type.get_size())) {
              return Expression::Error(
                  Expression::Error::Type::ArithmeticOverflow, *this);
            }
          } else {
            value = left_value->get_value() % divisor;
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

    return selected.visit<Ttx::Model::Types::Unsigned>(
        [&](const Ttx::Model::Types::Unsigned& type)
            -> Utility::Result<Core::Option<Constant&>, Expression::Error> {
          Unsigned_64 divisor = right_value->get_value();
          if (divisor == 0) {
            return Expression::Error(
                Expression::Error::Type::DivisionByZero, *this);
          }

          Unsigned_64 value = left_value->get_value() % divisor;
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

  return Expression::Error(
      Expression::Error::Type::InvalidOperationType, *this);
}
