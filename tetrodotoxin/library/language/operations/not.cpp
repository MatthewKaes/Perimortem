// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/language/operations/not.hpp"

#include "perimortem/core/static/vector.hpp"

#include "tetrodotoxin/library/dialect.hpp"
#include "tetrodotoxin/library/language/constants/false.hpp"
#include "tetrodotoxin/library/language/constants/flag.hpp"
#include "tetrodotoxin/library/language/constants/true.hpp"
#include "tetrodotoxin/library/language/parser/expression.hpp"
#include "ttx/concept/invalid.hpp"

using namespace Perimortem;
using namespace Tetrodotoxin::Library;
using namespace Ttx::Concept;
using namespace Ttx::Lexical;
using namespace Ttx::Model;

static auto select_result_type(const Language::Expression& operand)
    -> const Abstract& {
  const Abstract& selected = operand.get_type().resolve();
  if (&selected != &Dialect::get_bool()) {
    return Invalid::get_invalid();
  }

  return selected;
}

static auto report_type(
    Cursor& cursor,
    Span span,
    const Language::Expression& operand) -> void {
  auto report = cursor.create_report(span);
  report << "Not cannot use Type `"_view << operand.get_type().get_name()
         << "`."_view;
  report.get_hint() << "Use a value with the exact canonical Bool Type."_view;
}

static auto report_fold_error(
    Cursor& cursor,
    Span span,
    const Language::Operations::Not& operation,
    const Language::Expression& operand,
    const Language::FoldError& error) -> void {
  switch (error.get_type()) {
  case Language::FoldError::Type::InvalidOperationType:
    if (&error.get_expression() == &operation) {
      report_type(cursor, span, operand);
      return;
    }
    break;
  default:
    break;
  }

  auto report = cursor.create_report(span);
  report << "Not input `"_view << error.get_expression().get_name()
         << "` failed folding with "_view << error.get_name() << "."_view;
  report.get_hint()
      << "Check that input operation and its explicit result Type."_view;
}

static auto make_result(Memory::Allocator::Arena& domain, Bool value)
    -> const Language::Expression& {
  if (value) {
    return domain.construct<Language::Constants::True>(Dialect::get_bool());
  }

  return domain.construct<Language::Constants::False>(Dialect::get_bool());
}

auto Language::Operations::Not::parse(
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
        span, "Not has a malformed operand."_view,
        "Use a complete Expression after unary `!`."_view);
    return {};
  }

  const auto& operation = domain.construct<Not>(domain, *operand);
  if (!operation.get_type().resolve().is<Ttx::Model::Types::Flag>()) {
    report_type(cursor, span, *operand);
    return {};
  }

  auto folded = operation.attempt_fold(domain, materializations);
  return folded.visit(
      [](const Expression& expression) -> Utility::Option<const Expression&> {
        return expression;
      },
      [&](const FoldError& error) -> Utility::Option<const Expression&> {
        report_fold_error(cursor, span, operation, *operand, error);
        return {};
      });
}

Language::Operations::Not::Not(
    Memory::Allocator::Arena& domain,
    const Expression& operand)
    : Operation(
          domain,
          Core::Static::Vector<Ttx::Concept::Reference<Expression>, 1>{
            {operand}}),
      result_type(select_result_type(operand)) {}

auto Language::Operations::Not::get_documentation() const
    -> const Documentation& {
  return Documentation::get_empty();
}

auto Language::Operations::Not::get_type() const -> const Abstract& {
  return result_type;
}

auto Language::Operations::Not::evaluate_constants(
    Memory::Allocator::Arena& domain,
    Materializations&) const -> Utility::Result<const Expression&, FoldError> {
  auto operand = get_input(0);
  if (!operand) {
    return FoldError(FoldError::Type::InvalidInput, *this);
  }

  // Exact Bool identity was fixed before folding. Flag proves the completed
  // payload while True and False remain the canonical published results.
  auto value = operand->visit<Constants::Flag>(
      [](const Constants::Flag& selected)
          -> Utility::Option<const Constants::Flag&> { return selected; },
      [](const Abstract&) -> Utility::Option<const Constants::Flag&> {
        return {};
      });
  if (!value) {
    return FoldError(FoldError::Type::InvalidConstant, *operand);
  }

  return make_result(domain, !value->get_value());
}
