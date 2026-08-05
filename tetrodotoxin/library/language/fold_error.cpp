// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/language/fold_error.hpp"

using namespace Perimortem::Core;
using namespace Tetrodotoxin::Library;

Language::FoldError::FoldError(Type type, const Expression& expression)
    : type(type), expression(expression) {}

auto Language::FoldError::get_type() const -> Type {
  return type;
}

auto Language::FoldError::get_expression() const -> const Expression& {
  return expression.get();
}

auto Language::FoldError::get_name() const -> View::Bytes {
  switch (type) {
  case Type::InvalidOperationType:
    return "invalid operation Type"_view;
  case Type::InvalidInput:
    return "invalid operation input"_view;
  case Type::InvalidConstant:
    return "invalid Constant domain"_view;
  case Type::ResultTypeMismatch:
    return "changed result Type"_view;
  case Type::NegativeOperand:
    return "negative operand"_view;
  case Type::CountOverflow:
    return "Count or Fixed extent overflow"_view;
  case Type::IndexOutOfBounds:
    return "index outside the receiver"_view;
  case Type::RangeStartOutOfBounds:
    return "range start outside the receiver"_view;
  case Type::RangeSizeOutOfBounds:
    return "range size outside the receiver"_view;
  case Type::ArithmeticOverflow:
    return "arithmetic overflow"_view;
  case Type::Unknown:
    return "unknown fold failure"_view;
  }

  return "unknown fold failure"_view;
}
