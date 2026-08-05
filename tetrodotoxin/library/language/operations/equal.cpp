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

static auto is_scalar_type(const Abstract& selected) -> Bool {
  return selected.is<Ttx::Model::Types::Signed>() ||
         selected.is<Ttx::Model::Types::Unsigned>() ||
         selected.is<Ttx::Model::Types::Real>() ||
         selected.is<Ttx::Model::Types::Flag>();
}

static auto select_operand_type(
    const Language::Expression& left,
    const Language::Expression& right) -> const Abstract& {
  const Abstract& left_resolved = left.get_type().resolve();
  const Abstract& right_resolved = right.get_type().resolve();
  if (!left_resolved.is<Type>() || &left_resolved != &right_resolved) {
    return Invalid::get_invalid();
  }

  if (is_scalar_type(left_resolved) ||
      (left.is<Language::Constants::Bytes>() &&
       right.is<Language::Constants::Bytes>())) {
    return left_resolved;
  }

  // Bytes is a complete Constant payload domain rather than a universal Type
  // category. Admitting both values here keeps that ownership distinction.
  return Invalid::get_invalid();
}

static auto report_types(
    Cursor& cursor,
    Span span,
    const Language::Expression& left,
    const Language::Expression& right) -> void {
  auto report = cursor.create_report(span);
  report << "Equal cannot use left Type `"_view << left.get_type().get_name()
         << "` with right Type `"_view << right.get_type().get_name()
         << "`."_view;
  report.get_hint()
      << "Use exact matching scalar Types or complete Bytes values."_view;
}

static auto report_fold_error(
    Cursor& cursor,
    Span span,
    const Language::Operations::Equal& equal,
    const Language::Expression& left,
    const Language::Expression& right,
    const Language::FoldError& error) -> void {
  switch (error.get_type()) {
  case Language::FoldError::Type::InvalidOperationType:
    if (&error.get_expression() == &equal) {
      report_types(cursor, span, left, right);
      return;
    }
    break;
  default:
    break;
  }

  auto report = cursor.create_report(span);
  report << "Equal input `"_view << error.get_expression().get_name()
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

static auto select_constant(const Language::Expression& expression)
    -> Utility::Option<const Language::Constant&> {
  return expression.visit<Language::Constant>(
      [](const Language::Constant& selected)
          -> Utility::Option<const Language::Constant&> { return selected; },
      [](const Abstract&) -> Utility::Option<const Language::Constant&> {
        return {};
      });
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
    const Expression& left) -> Utility::Option<const Expression&> {
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

  const auto& equal = domain.construct<Equal>(domain, left, *right);
  if (!equal.get_type().resolve().is<Ttx::Model::Types::Flag>()) {
    report_types(transaction, span, left, *right);
    return {};
  }

  auto folded = equal.attempt_fold(domain, materializations);
  auto parsed = folded.visit(
      [](const Expression& expression) -> Utility::Option<const Expression&> {
        return expression;
      },
      [&](const FoldError& error) -> Utility::Option<const Expression&> {
        report_fold_error(transaction, span, equal, left, *right, error);
        return {};
      });
  if (!parsed) {
    return {};
  }

  // The branch may publish diagnostics, but caller position changes only after
  // grammar, domain legality, and eager folding complete successfully.
  cursor.join(transaction);
  return *parsed;
}

Language::Operations::Equal::Equal(
    Memory::Allocator::Arena& domain,
    const Expression& left,
    const Expression& right)
    : Operation(
          domain,
          Core::Static::Vector<Ttx::Concept::Reference<Expression>, 2>{
            {left, right}}),
      operand_type(select_operand_type(left, right)) {}

auto Language::Operations::Equal::get_documentation() const
    -> const Documentation& {
  return Documentation::get_empty();
}

auto Language::Operations::Equal::get_type() const -> const Abstract& {
  if (operand_type.resolve().is<Type>()) {
    return Dialect::get_bool();
  }

  return Invalid::get_invalid();
}

auto Language::Operations::Equal::evaluate_constants(
    Memory::Allocator::Arena& domain,
    Materializations&) const -> Utility::Result<const Expression&, FoldError> {
  const Abstract& selected = operand_type.resolve();
  auto left = get_input(0);
  auto right = get_input(1);
  if (!left || !right) {
    return FoldError(FoldError::Type::InvalidInput, *this);
  }

  // Constant owns each payload comparison and exact Type identity. Equal only
  // proves that the completed inputs still belong to the selected domain.
  auto left_value = select_constant(*left);
  auto right_value = select_constant(*right);
  if (!left_value || !matches_domain(*left, selected)) {
    return FoldError(FoldError::Type::InvalidConstant, *left);
  }

  if (!right_value || !matches_domain(*right, selected)) {
    return FoldError(FoldError::Type::InvalidConstant, *right);
  }

  return make_result(domain, *left_value == *right_value);
}
