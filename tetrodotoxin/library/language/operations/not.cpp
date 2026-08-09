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

static auto make_result(Memory::Allocator::Arena& domain, Bool value)
    -> Language::Constant& {
  if (value) {
    return Language::Constants::True::create_synthetic(
        domain, Dialect::get_bool());
  }

  return Language::Constants::False::create_synthetic(
      domain, Dialect::get_bool());
}

auto Language::Operations::Not::parse(
    Memory::Allocator::Arena& domain,
    Materializations& materializations,
    Cursor& cursor,
    const Abstract& source_context) -> Core::Option<Expression&> {
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

  const auto& operand_anchor = operand->get_anchor();
  if (!operand_anchor) {
    cursor.create_expression_error(
        span, "Not requires an authored operand Anchor."_view);
    return {};
  }

  auto anchor =
      Anchor::create(opening, Span(opening), operand_anchor->get_span());
  return create_authored(domain, materializations, *operand, anchor);
}

auto Language::Operations::Not::create_authored(
    Memory::Allocator::Arena& domain,
    Materializations& materializations,
    Expression& operand,
    Anchor anchor) -> Not& {
  return Expression::create_authored<Not>(
      domain, anchor, [&](auto source) -> Not {
        return Not(domain, materializations, operand, source);
      });
}

auto Language::Operations::Not::create_synthetic(
    Memory::Allocator::Arena& domain,
    Materializations& materializations,
    Expression& operand) -> Not& {
  return Expression::create_synthetic<Not>(domain, [&](auto source) -> Not {
    return Not(domain, materializations, operand, source);
  });
}

Language::Operations::Not::Not(
    Memory::Allocator::Arena& domain,
    Materializations& materializations,
    Expression& operand,
    Core::Option<Anchor> anchor)
    : Operation(
          domain,
          materializations,
          Core::Static::Vector<Ttx::Concept::Reference<Expression>, 1>{
            {operand}},
          anchor) {}

auto Language::Operations::Not::get_documentation() const
    -> const Documentation& {
  return Documentation::get_empty();
}

auto Language::Operations::Not::select_type(Materializations&) const
    -> Core::Option<const Type&> {
  auto operand = get_input(0);
  if (!operand) {
    return {};
  }

  return select_result_type(*operand).select<Type>();
}

auto Language::Operations::Not::evaluate_constants(
    Memory::Allocator::Arena& domain,
    Materializations&)
    -> Utility::Result<Core::Option<Constant&>, Expression::Error> {
  auto authored_operand = get_input(0);
  auto operand = get_folded_input(0);
  if (!authored_operand || !operand) {
    return Expression::Error(Expression::Error::Type::InvalidInput, *this);
  }

  // Exact Bool identity was fixed before folding. Flag proves the completed
  // payload while True and False remain the canonical published results.
  auto value = operand->select<Constants::Flag>();
  if (!value) {
    return Expression::Error(
        Expression::Error::Type::InvalidConstant, *authored_operand);
  }

  return make_result(domain, !value->get_value());
}
