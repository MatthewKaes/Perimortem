// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/language/operations/negate.hpp"

#include "perimortem/core/static/vector.hpp"
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

static auto report_type(
    Cursor& cursor,
    Span span,
    const Language::Expression& operand) -> void {
  auto report = cursor.create_report(span);
  report << "Negate cannot use Type `"_view << operand.get_type().get_name()
         << "`."_view;
  report.get_hint() << "Use a value with an explicit Signed or Real Type."_view;
}

static auto report_overflow(
    Cursor& cursor,
    Span span,
    const Language::Operations::Negate& negate,
    const Language::Expression& operand) -> void {
  auto report = cursor.create_report(span);
  report << "Negate cannot represent "_view;
  operand.visit<Language::Constants::Signed>(
      [&](const Language::Constants::Signed& value) {
        report << "the inverse of "_view << value.get_value();
      },
      [](const Abstract&) {});
  report << " in selected Type `"_view << negate.get_type().get_name()
         << "`."_view;
  report.get_hint()
      << "Use a value whose inverse fits the selected Type width."_view;
}

static auto report_fold_error(
    Cursor& cursor,
    Span span,
    const Language::Operations::Negate& negate,
    const Language::Expression& operand,
    const Language::FoldError& error) -> void {
  switch (error.get_type()) {
  case Language::FoldError::Type::InvalidOperationType:
    if (&error.get_expression() == &negate) {
      report_type(cursor, span, operand);
      return;
    }
    break;
  case Language::FoldError::Type::ArithmeticOverflow:
    if (&error.get_expression() == &negate) {
      report_overflow(cursor, span, negate, operand);
      return;
    }
    break;
  default:
    break;
  }

  auto report = cursor.create_report(span);
  report << "Negate input `"_view << error.get_expression().get_name()
         << "` failed folding with "_view << error.get_name() << "."_view;
  report.get_hint()
      << "Check that input operation and its explicit result Type."_view;
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

auto Language::Operations::Negate::parse(
    Memory::Allocator::Arena& domain,
    Materializations& materializations,
    Cursor& cursor,
    const Abstract& source_context) -> Utility::Option<const Expression&> {
  Token opening = cursor.consume();
  auto operand = Language::Parser::Expression::parse_prefix_operand(
      domain, materializations, cursor, source_context);
  Span span(opening, cursor.peek(-1));
  if (!operand) {
    cursor.create_expression_error(
        span, "Negate has a malformed operand."_view,
        "Use a complete Expression after unary `-`."_view);
    return {};
  }

  const auto& negate = domain.construct<Negate>(domain, *operand);
  if (!negate.get_type().resolve().is<Type>()) {
    report_type(cursor, span, *operand);
    return {};
  }

  auto folded = negate.attempt_fold(domain, materializations);
  return folded.visit(
      [](const Expression& expression) -> Utility::Option<const Expression&> {
        return expression;
      },
      [&](const FoldError& error) -> Utility::Option<const Expression&> {
        report_fold_error(cursor, span, negate, *operand, error);
        return {};
      });
}

Language::Operations::Negate::Negate(
    Memory::Allocator::Arena& domain,
    const Expression& operand)
    : Operation(
          domain,
          Core::Static::Vector<Ttx::Concept::Reference<Expression>, 1>{
            {operand}}),
      result_type(select_result_type(operand)) {}

auto Language::Operations::Negate::get_documentation() const
    -> const Documentation& {
  return Documentation::get_empty();
}

auto Language::Operations::Negate::get_type() const -> const Abstract& {
  return result_type;
}

auto Language::Operations::Negate::evaluate_constants(
    Memory::Allocator::Arena& domain,
    Materializations&) const -> Utility::Result<const Expression&, FoldError> {
  const Abstract& selected = result_type.resolve();
  auto operand = get_input(0);
  if (!operand) {
    return FoldError(FoldError::Type::InvalidInput, *this);
  }

  // Construction fixes the exact result Type before folding. The visitors
  // prove only the Constant payload needed to calculate its inverse.
  if (selected.is<Ttx::Model::Types::Signed>()) {
    auto value = select_constant<Constants::Signed>(*operand);
    if (!value) {
      return FoldError(FoldError::Type::InvalidConstant, *operand);
    }

    return selected.visit<Ttx::Model::Types::Signed>(
        [&](const Ttx::Model::Types::Signed& type)
            -> Utility::Result<const Expression&, FoldError> {
          Signed_64 inverse = 0;
          if (!signed_inverse(type, value->get_value(), inverse)) {
            return FoldError(FoldError::Type::ArithmeticOverflow, *this);
          }

          return domain.construct<Constants::Signed>(type, inverse);
        },
        [&](const Abstract&) -> Utility::Result<const Expression&, FoldError> {
          return FoldError(FoldError::Type::InvalidOperationType, *this);
        });
  }

  if (selected.is<Ttx::Model::Types::Real>()) {
    auto value = select_constant<Constants::Real>(*operand);
    if (!value) {
      return FoldError(FoldError::Type::InvalidConstant, *operand);
    }

    return selected.visit<Ttx::Model::Types::Real>(
        [&](const Ttx::Model::Types::Real& type)
            -> Utility::Result<const Expression&, FoldError> {
          if (type.get_size() == sizeof(Real_32)) {
            Real_32 inverse = -Real_32(value->get_value());
            return domain.construct<Constants::Real>(type, Real_64(inverse));
          }

          if (type.get_size() == sizeof(Real_64)) {
            return domain.construct<Constants::Real>(type, -value->get_value());
          }

          return FoldError(FoldError::Type::InvalidOperationType, *this);
        },
        [&](const Abstract&) -> Utility::Result<const Expression&, FoldError> {
          return FoldError(FoldError::Type::InvalidOperationType, *this);
        });
  }

  return FoldError(FoldError::Type::InvalidOperationType, *this);
}
