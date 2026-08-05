// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/language/operations/divide.hpp"

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

  // Divide keeps both authored Types exact. A receiving typed owner performs
  // any conversion before construction so runtime and Constant inputs agree.
  return left_resolved;
}

static auto report_types(
    Cursor& cursor,
    Span span,
    const Language::Expression& left,
    const Language::Expression& right) -> void {
  auto report = cursor.create_report(span);
  report << "Divide cannot use left Type `"_view << left.get_type().get_name()
         << "` with right Type `"_view << right.get_type().get_name()
         << "`."_view;
  report.get_hint()
      << "Use two values with the same explicit numeric Type."_view;
}

static auto report_zero(
    Cursor& cursor,
    Span span,
    const Language::Operations::Divide& divide) -> void {
  auto report = cursor.create_report(span);
  report
      << "Divide cannot use zero as an integer divisor for selected Type `"_view
      << divide.get_type().get_name() << "`."_view;
  report.get_hint() << "Use a nonzero integer divisor."_view;
}

static auto report_overflow(
    Cursor& cursor,
    Span span,
    const Language::Operations::Divide& divide,
    const Language::Expression& left,
    const Language::Expression& right) -> void {
  auto report = cursor.create_report(span);
  report << "Divide cannot represent "_view;
  left.visit<Language::Constants::Signed>(
      [&](const Language::Constants::Signed& left_value) {
        right.visit<Language::Constants::Signed>(
            [&](const Language::Constants::Signed& right_value) {
              report << left_value.get_value() << " divided by "_view
                     << right_value.get_value();
            },
            [](const Abstract&) {});
      },
      [](const Abstract&) {});
  report << " in selected Type `"_view << divide.get_type().get_name()
         << "`."_view;
  report.get_hint()
      << "Use values whose quotient fits the selected Type width."_view;
}

static auto report_fold_error(
    Cursor& cursor,
    Span span,
    const Language::Operations::Divide& divide,
    const Language::Expression& left,
    const Language::Expression& right,
    const Language::FoldError& error) -> void {
  switch (error.get_type()) {
  case Language::FoldError::Type::InvalidOperationType:
    if (&error.get_expression() == &divide) {
      report_types(cursor, span, left, right);
      return;
    }
    break;
  case Language::FoldError::Type::DivisionByZero:
    if (&error.get_expression() == &divide) {
      report_zero(cursor, span, divide);
      return;
    }
    break;
  case Language::FoldError::Type::ArithmeticOverflow:
    if (&error.get_expression() == &divide) {
      report_overflow(cursor, span, divide, left, right);
      return;
    }
    break;
  default:
    break;
  }

  auto report = cursor.create_report(span);
  report << "Divide input `"_view << error.get_expression().get_name()
         << "` failed folding with "_view << error.get_name() << "."_view;
  report.get_hint()
      << "Check that input operation and its explicit result Type."_view;
}

auto Language::Operations::Divide::parse(
    Memory::Allocator::Arena& domain,
    Materializations& materializations,
    Cursor& cursor,
    const Abstract& source_context,
    const Expression& left) -> Utility::Option<const Expression&> {
  auto transaction = cursor.branch();
  Token opening = transaction.consume();
  auto right = Language::Parser::Expression::parse_operand(
      domain, materializations, transaction, source_context, Code::Type::DivOp);
  Span span(opening, transaction.peek(-1));
  if (!right) {
    transaction.create_expression_error(
        span, "Divide has a malformed right operand."_view,
        "Use a complete scalar Expression after `/`."_view);
    return {};
  }

  const auto& divide = domain.construct<Divide>(domain, left, *right);
  if (!divide.get_type().resolve().is<Type>()) {
    report_types(transaction, span, left, *right);
    return {};
  }

  auto folded = divide.attempt_fold(domain, materializations);
  auto parsed = folded.visit(
      [](const Expression& expression) -> Utility::Option<const Expression&> {
        return expression;
      },
      [&](const FoldError& error) -> Utility::Option<const Expression&> {
        report_fold_error(transaction, span, divide, left, *right, error);
        return {};
      });
  if (!parsed) {
    return {};
  }

  // The concrete grammar may publish diagnostics through its branch, but it
  // commits position only after grammar, legality, and folding all succeed.
  cursor.join(transaction);
  return *parsed;
}

Language::Operations::Divide::Divide(
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

auto Language::Operations::Divide::get_documentation() const
    -> const Documentation& {
  return Documentation::get_empty();
}

auto Language::Operations::Divide::get_type() const -> const Abstract& {
  return result_type;
}

auto Language::Operations::Divide::evaluate_constants(
    Memory::Allocator::Arena& domain,
    Materializations&) const -> Utility::Result<const Expression&, FoldError> {
  const Abstract& selected = result_type.resolve();
  auto left = get_input(0);
  auto right = get_input(1);
  if (!left || !right) {
    return FoldError(FoldError::Type::InvalidInput, *this);
  }

  // Type selection fixes the payload domain before folding. Integer guards
  // run before host arithmetic so zero and the signed endpoint stay failures.
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
          Signed_64 divisor = right_value->get_value();
          if (divisor == 0) {
            return FoldError(FoldError::Type::DivisionByZero, *this);
          }

          Signed_64 value = 0;
          if (divisor == -1) {
            Bool overflow = __builtin_sub_overflow(
                Signed_64(0), left_value->get_value(), &value);
            if (overflow ||
                !Core::Math::is_representable(value, type.get_size())) {
              return FoldError(FoldError::Type::ArithmeticOverflow, *this);
            }
          } else {
            value = left_value->get_value() / divisor;
          }

          if (!Core::Math::is_representable(value, type.get_size())) {
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
          Unsigned_64 divisor = right_value->get_value();
          if (divisor == 0) {
            return FoldError(FoldError::Type::DivisionByZero, *this);
          }

          Unsigned_64 value = left_value->get_value() / divisor;
          if (!Core::Math::is_representable(value, type.get_size())) {
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
            Real_32 value = Real_32(left_value->get_value()) /
                            Real_32(right_value->get_value());
            return domain.construct<Constants::Real>(type, Real_64(value));
          }

          if (type.get_size() == sizeof(Real_64)) {
            return domain.construct<Constants::Real>(
                type, left_value->get_value() / right_value->get_value());
          }

          return FoldError(FoldError::Type::InvalidOperationType, *this);
        },
        [&](const Abstract&) -> Utility::Result<const Expression&, FoldError> {
          return FoldError(FoldError::Type::InvalidOperationType, *this);
        });
  }

  return FoldError(FoldError::Type::InvalidOperationType, *this);
}
