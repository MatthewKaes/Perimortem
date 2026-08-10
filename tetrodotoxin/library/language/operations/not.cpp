// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/language/operations/not.hpp"

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

TTX_DIRECT_UNARY_PARSE(
    Not,
    "Not has a malformed operand."_view,
    "Use a complete Expression after unary `!`."_view,
    "Not requires an authored operand Anchor."_view);

TTX_UNARY_OP(Not);

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
