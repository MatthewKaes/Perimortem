// Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/library/language/operations/not_equal.hpp"

#include "tetrodotoxin/library/language/constants/bytes.hpp"
#include "tetrodotoxin/library/language/constants/false.hpp"
#include "tetrodotoxin/library/language/constants/true.hpp"
#include "tetrodotoxin/library/language/model/types/flag.hpp"
#include "tetrodotoxin/library/language/model/types/value.hpp"
#include "tetrodotoxin/library/language/parser/expression.hpp"
#include "tetrodotoxin/library/language/types/view.hpp"
#include "ttx/concept/invalid.hpp"

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
  auto left_type = left_resolved.select<Language::Model::Type>();
  auto right_type = right_resolved.select<Language::Model::Type>();
  if (!left_type || !right_type) {
    return Invalid::get_invalid();
  }

  if (&left_resolved == &right_resolved &&
      (left_resolved
           .is<Tetrodotoxin::Library::Language::Model::Types::Value>() ||
       (left.is<Language::Constants::Bytes>() &&
        right.is<Language::Constants::Bytes>()))) {
    return left_resolved;
  }

  auto left_view = left_type->select<Language::Types::View>();
  auto right_view = right_type->select<Language::Types::View>();
  if (left_view && right_view && left_view->accepts(right) &&
      right_view->accepts(left)) {
    return *left_view;
  }

  // Bytes is a complete Constant payload domain rather than a universal Type
  // category. Admitting both values here keeps that ownership distinction.
  return Invalid::get_invalid();
}

static auto make_result(
    Memory::Allocator::Arena& domain,
    const Tetrodotoxin::Library::Language::Model::Types::Flag& type,
    Bool value) -> Language::Constant& {
  if (value) {
    return Language::Constants::True::create_synthetic(domain, type);
  }

  return Language::Constants::False::create_synthetic(domain, type);
}

static auto accepts_constant(
    const Abstract& selected,
    const Language::Constant& constant) -> Bool {
  auto scalar =
      selected.select<Tetrodotoxin::Library::Language::Model::Types::Value>();
  if (scalar) {
    return scalar->accepts_constant(constant);
  }

  auto type = selected.select<Language::Model::Type>();
  return type && constant.is<Language::Constants::Bytes>() &&
         type->accepts(constant);
}

TTX_BINARY_PARSE(NotEqual, NotEqOp);

TTX_BINARY_OP(NotEqual);

auto Language::Operations::NotEqual::select_type(
    const Ttx::Concept::Abstract& context) const
    -> Core::Option<const Language::Model::Type&> {
  auto inputs = get_inputs();
  const Expression& left = inputs.get_data()[0].get();
  const Expression& right = inputs.get_data()[1].get();
  if (!select_operand_type(left, right).resolve().is<Language::Model::Type>()) {
    return {};
  }

  return context.resolve_context("Bool"_view).select<Language::Model::Type>();
}

auto Language::Operations::NotEqual::evaluate_constants(
    Memory::Allocator::Arena& domain)
    -> Utility::Result<Core::Option<Constant&>, Expression::Error> {
  auto inputs = get_inputs();
  Expression& authored_left = inputs.get_data()[0].get();
  Expression& authored_right = inputs.get_data()[1].get();
  auto left = get_folded_input(0);
  auto right = get_folded_input(1);
  auto result_type =
      get_type().select<Tetrodotoxin::Library::Language::Model::Types::Flag>();
  if (!left || !right || !result_type) {
    return Expression::Error(Expression::Error::Type::InvalidInput, *this);
  }

  const Abstract& selected = select_operand_type(authored_left, authored_right);

  // The scalar Type proves its own Constant carrier. Bytes remains the one
  // complete non scalar Constant domain admitted by this operator.
  auto left_value = left->select<Constant>();
  auto right_value = right->select<Constant>();
  if (!left_value || !accepts_constant(selected, *left_value)) {
    return Expression::Error(
        Expression::Error::Type::InvalidConstant, authored_left);
  }

  if (!right_value || !accepts_constant(selected, *right_value)) {
    return Expression::Error(
        Expression::Error::Type::InvalidConstant, authored_right);
  }

  return make_result(domain, *result_type, *left_value != *right_value);
}
