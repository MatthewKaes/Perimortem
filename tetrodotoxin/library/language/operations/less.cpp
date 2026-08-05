// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/language/operations/less.hpp"

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

  // Two runtime values and two Constants follow the same rule. A Literal keeps
  // its binary wide Type until an explicitly typed owner performs conversion.
  return left_resolved;
}

static auto report_types(
    Cursor& cursor,
    Span span,
    const Language::Expression& left,
    const Language::Expression& right) -> void {
  auto report = cursor.create_report(span);
  report << "Less cannot use left Type `"_view << left.get_type().get_name()
         << "` with right Type `"_view << right.get_type().get_name()
         << "`."_view;
  report.get_hint()
      << "Use two values with the same explicit numeric Type."_view;
}

static auto report_fold_error(
    Cursor& cursor,
    Span span,
    const Language::Operations::Less& less,
    const Language::Expression& left,
    const Language::Expression& right,
    const Language::FoldError& error) -> void {
  switch (error.get_type()) {
  case Language::FoldError::Type::InvalidOperationType:
    if (&error.get_expression() == &less) {
      report_types(cursor, span, left, right);
      return;
    }
    break;
  default:
    break;
  }

  auto report = cursor.create_report(span);
  report << "Less input `"_view << error.get_expression().get_name()
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

auto Language::Operations::Less::parse(
    Memory::Allocator::Arena& domain,
    Materializations& materializations,
    Cursor& cursor,
    const Abstract& source_context,
    const Expression& left) -> Utility::Option<const Expression&> {
  Token opening = cursor.consume();
  auto right = Language::Parser::Expression::parse_operand(
      domain, materializations, cursor, source_context, Code::Type::LessOp);
  Span span(opening, cursor.peek(-1));
  if (!right) {
    cursor.create_expression_error(
        span, "Less has a malformed right operand."_view,
        "Use a complete scalar Expression after `<`."_view);
    return {};
  }

  const auto& less = domain.construct<Less>(domain, left, *right);
  if (!less.get_type().resolve().is<Ttx::Model::Types::Flag>()) {
    report_types(cursor, span, left, *right);
    return {};
  }

  auto folded = less.attempt_fold(domain, materializations);
  return folded.visit(
      [](const Expression& expression) -> Utility::Option<const Expression&> {
        return expression;
      },
      [&](const FoldError& error) -> Utility::Option<const Expression&> {
        report_fold_error(cursor, span, less, left, *right, error);
        return {};
      });
}

Language::Operations::Less::Less(
    Memory::Allocator::Arena& domain,
    const Expression& left,
    const Expression& right)
    : Operation(
          domain,
          Core::Static::Vector<Ttx::Concept::Reference<Expression>, 2>{
            {left, right}}),
      operand_type(select_operand_type(left, right)) {}

auto Language::Operations::Less::get_documentation() const
    -> const Documentation& {
  return Documentation::get_empty();
}

auto Language::Operations::Less::get_type() const -> const Abstract& {
  if (operand_type.resolve().is<Type>()) {
    return Dialect::get_bool();
  }

  return Invalid::get_invalid();
}

auto Language::Operations::Less::evaluate_constants(
    Memory::Allocator::Arena& domain,
    Materializations&) const -> Utility::Result<const Expression&, FoldError> {
  const Abstract& selected = operand_type.resolve();
  auto left = get_input(0);
  auto right = get_input(1);
  if (!left || !right) {
    return FoldError(FoldError::Type::InvalidInput, *this);
  }

  // The retained operand domain was established before folding. Visitors prove
  // the matching Constant payload while every comparison publishes canonical
  // Bool identity regardless of that numeric domain.
  if (selected.is<Ttx::Model::Types::Signed>()) {
    auto left_value = select_constant<Constants::Signed>(*left);
    auto right_value = select_constant<Constants::Signed>(*right);
    if (!left_value) {
      return FoldError(FoldError::Type::InvalidConstant, *left);
    }

    if (!right_value) {
      return FoldError(FoldError::Type::InvalidConstant, *right);
    }

    return make_result(
        domain, left_value->get_value() < right_value->get_value());
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

    return make_result(
        domain, left_value->get_value() < right_value->get_value());
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
            return make_result(
                domain, Real_32(left_value->get_value()) <
                            Real_32(right_value->get_value()));
          }

          if (type.get_size() == sizeof(Real_64)) {
            return make_result(
                domain, left_value->get_value() < right_value->get_value());
          }

          return FoldError(FoldError::Type::InvalidOperationType, *this);
        },
        [&](const Abstract&) -> Utility::Result<const Expression&, FoldError> {
          return FoldError(FoldError::Type::InvalidOperationType, *this);
        });
  }

  return FoldError(FoldError::Type::InvalidOperationType, *this);
}
