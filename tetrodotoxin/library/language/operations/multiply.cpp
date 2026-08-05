// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/language/operations/multiply.hpp"

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

static auto select_result_type(
    const Language::Expression& left,
    const Language::Expression& right) -> const Abstract& {
  const Abstract& left_resolved = left.get_type().resolve();
  const Abstract& right_resolved = right.get_type().resolve();
  if (!left_resolved.is<Type>() || &left_resolved != &right_resolved ||
      !is_numeric_type(left_resolved)) {
    return Invalid::get_invalid();
  }

  // Literal Constants keep their declared Type here. Conversion belongs to a
  // receiving typed owner, so runtime values and completed values follow the
  // same exact identity rule.
  return left_resolved;
}

static auto report_types(
    Cursor& cursor,
    Span span,
    const Language::Expression& left,
    const Language::Expression& right) -> void {
  auto report = cursor.create_report(span);
  report << "Multiply cannot use left Type `"_view << left.get_type().get_name()
         << "` with right Type `"_view << right.get_type().get_name()
         << "`."_view;
  report.get_hint()
      << "Use two values with the same explicit numeric Type."_view;
}

static auto report_overflow(
    Cursor& cursor,
    Span span,
    const Language::Operations::Multiply& multiply,
    const Language::Expression& left,
    const Language::Expression& right) -> void {
  auto report = cursor.create_report(span);
  report << "Multiply overflows selected Type `"_view
         << multiply.get_type().get_name() << "` for values "_view;
  left.visit<Language::Constants::Signed>(
      [&](const Language::Constants::Signed& left_value) {
        right.visit<Language::Constants::Signed>(
            [&](const Language::Constants::Signed& right_value) {
              report << left_value.get_value() << " and "_view
                     << right_value.get_value();
            },
            [](const Abstract&) {});
      },
      [&](const Abstract& left_expression) {
        left_expression.visit<Language::Constants::Unsigned>(
            [&](const Language::Constants::Unsigned& left_value) {
              right.visit<Language::Constants::Unsigned>(
                  [&](const Language::Constants::Unsigned& right_value) {
                    report << left_value.get_value() << " and "_view
                           << right_value.get_value();
                  },
                  [](const Abstract&) {});
            },
            [](const Abstract&) {});
      });

  report << "."_view;
  report.get_hint()
      << "Use values representable by the selected Type width."_view;
}

static auto report_fold_error(
    Cursor& cursor,
    Span span,
    const Language::Operations::Multiply& multiply,
    const Language::Expression& left,
    const Language::Expression& right,
    const Language::FoldError& error) -> void {
  switch (error.get_type()) {
  case Language::FoldError::Type::InvalidOperationType:
    if (&error.get_expression() == &multiply) {
      report_types(cursor, span, left, right);
      return;
    }
    break;
  case Language::FoldError::Type::ArithmeticOverflow:
    if (&error.get_expression() == &multiply) {
      report_overflow(cursor, span, multiply, left, right);
      return;
    }
    break;
  default:
    break;
  }

  auto report = cursor.create_report(span);
  report << "Multiply input `"_view << error.get_expression().get_name()
         << "` failed folding with "_view << error.get_name() << "."_view;
  report.get_hint()
      << "Check that input operation and its explicit result Type."_view;
}

static auto signed_product(
    const Ttx::Model::Types::Signed& type,
    Signed_64 left,
    Signed_64 right,
    Signed_64& result) -> Bool {
  if (__builtin_mul_overflow(left, right, &result)) {
    return False;
  }

  return Core::Math::is_representable(result, type.get_size());
}

static auto unsigned_product(
    const Ttx::Model::Types::Unsigned& type,
    Unsigned_64 left,
    Unsigned_64 right,
    Unsigned_64& result) -> Bool {
  if (__builtin_mul_overflow(left, right, &result)) {
    return False;
  }

  return Core::Math::is_representable(result, type.get_size());
}

auto Language::Operations::Multiply::parse(
    Memory::Allocator::Arena& domain,
    Materializations& materializations,
    Cursor& cursor,
    const Abstract& source_context,
    const Expression& left) -> Utility::Option<const Expression&> {
  Token opening = cursor.consume();
  auto right = Language::Parser::Expression::parse_operand(
      domain, materializations, cursor, source_context, Code::Type::MulOp);
  Span span(opening, cursor.peek(-1));
  if (!right) {
    cursor.create_expression_error(
        span, "Multiply has a malformed right operand."_view,
        "Use a complete scalar Expression after `*`."_view);
    return {};
  }

  const auto& multiply = domain.construct<Multiply>(domain, left, *right);
  if (!multiply.get_type().resolve().is<Type>()) {
    report_types(cursor, span, left, *right);
    return {};
  }

  auto folded = multiply.attempt_fold(domain, materializations);
  return folded.visit(
      [](const Expression& expression) -> Utility::Option<const Expression&> {
        return expression;
      },
      [&](const FoldError& error) -> Utility::Option<const Expression&> {
        report_fold_error(cursor, span, multiply, left, *right, error);
        return {};
      });
}

Language::Operations::Multiply::Multiply(
    Memory::Allocator::Arena& domain,
    const Expression& left,
    const Expression& right)
    : Operation(
          domain,
          Core::Static::Vector<Ttx::Concept::Reference<Expression>, 2>{{
            left,
            right,
          }}),
      result_type(select_result_type(left, right)) {}

auto Language::Operations::Multiply::get_documentation() const
    -> const Documentation& {
  return Documentation::get_empty();
}

auto Language::Operations::Multiply::get_type() const -> const Abstract& {
  return result_type;
}

auto Language::Operations::Multiply::evaluate_constants(
    Memory::Allocator::Arena& domain,
    Materializations&) const -> Utility::Result<const Expression&, FoldError> {
  const Abstract& selected = result_type.resolve();
  auto left = get_input(0);
  auto right = get_input(1);
  if (!left || !right) {
    return FoldError(FoldError::Type::InvalidInput, *this);
  }

  // Type legality has already fixed one exact domain before folding begins.
  // These visitors prove that completed payloads still implement that domain
  // instead of treating an error category as permission to cast them.
  if (selected.is<Ttx::Model::Types::Signed>()) {
    auto left_value = select_constant<Constants::Signed>(*left);
    auto right_value = select_constant<Constants::Signed>(*right);
    if (!left_value) {
      return FoldError(FoldError::Type::InvalidConstant, *left);
    }

    if (!right_value) {
      return FoldError(FoldError::Type::InvalidConstant, *right);
    }

    return selected.visit<Ttx::Model::Types::Signed>(
        [&](const Ttx::Model::Types::Signed& type)
            -> Utility::Result<const Expression&, FoldError> {
          Signed_64 value = 0;
          if (!signed_product(
                  type, left_value->get_value(), right_value->get_value(),
                  value)) {
            return FoldError(FoldError::Type::ArithmeticOverflow, *this);
          }

          return domain.construct<Constants::Signed>(type, value);
        },
        [&](const Abstract&) -> Utility::Result<const Expression&, FoldError> {
          return FoldError(FoldError::Type::InvalidOperationType, *this);
        });
  }

  if (selected.is<Ttx::Model::Types::Unsigned>()) {
    auto left_value = select_constant<Constants::Unsigned>(*left);
    auto right_value = select_constant<Constants::Unsigned>(*right);
    if (!left_value) {
      return FoldError(FoldError::Type::InvalidConstant, *left);
    }

    if (!right_value) {
      return FoldError(FoldError::Type::InvalidConstant, *right);
    }

    return selected.visit<Ttx::Model::Types::Unsigned>(
        [&](const Ttx::Model::Types::Unsigned& type)
            -> Utility::Result<const Expression&, FoldError> {
          Unsigned_64 value = 0;
          if (!unsigned_product(
                  type, left_value->get_value(), right_value->get_value(),
                  value)) {
            return FoldError(FoldError::Type::ArithmeticOverflow, *this);
          }

          return domain.construct<Constants::Unsigned>(type, value);
        },
        [&](const Abstract&) -> Utility::Result<const Expression&, FoldError> {
          return FoldError(FoldError::Type::InvalidOperationType, *this);
        });
  }

  if (selected.is<Ttx::Model::Types::Real>()) {
    auto left_value = select_constant<Constants::Real>(*left);
    auto right_value = select_constant<Constants::Real>(*right);
    if (!left_value) {
      return FoldError(FoldError::Type::InvalidConstant, *left);
    }

    if (!right_value) {
      return FoldError(FoldError::Type::InvalidConstant, *right);
    }

    return selected.visit<Ttx::Model::Types::Real>(
        [&](const Ttx::Model::Types::Real& type)
            -> Utility::Result<const Expression&, FoldError> {
          if (type.get_size() == sizeof(Real_32)) {
            Real_32 value = Real_32(left_value->get_value()) *
                            Real_32(right_value->get_value());
            return domain.construct<Constants::Real>(type, Real_64(value));
          }

          if (type.get_size() == sizeof(Real_64)) {
            return domain.construct<Constants::Real>(
                type, left_value->get_value() * right_value->get_value());
          }

          return FoldError(FoldError::Type::InvalidOperationType, *this);
        },
        [&](const Abstract&) -> Utility::Result<const Expression&, FoldError> {
          return FoldError(FoldError::Type::InvalidOperationType, *this);
        });
  }

  return FoldError(FoldError::Type::InvalidOperationType, *this);
}
