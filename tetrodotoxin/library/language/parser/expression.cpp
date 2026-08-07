// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/language/parser/expression.hpp"

#include "tetrodotoxin/library/language/identifier.hpp"
#include "tetrodotoxin/library/language/operations/divide.hpp"
#include "tetrodotoxin/library/language/operations/equal.hpp"
#include "tetrodotoxin/library/language/operations/greater.hpp"
#include "tetrodotoxin/library/language/operations/greater_equal.hpp"
#include "tetrodotoxin/library/language/operations/less.hpp"
#include "tetrodotoxin/library/language/operations/less_equal.hpp"
#include "tetrodotoxin/library/language/operations/modulo.hpp"
#include "tetrodotoxin/library/language/operations/multiply.hpp"
#include "tetrodotoxin/library/language/operations/negate.hpp"
#include "tetrodotoxin/library/language/operations/not.hpp"
#include "tetrodotoxin/library/language/operations/not_equal.hpp"
#include "tetrodotoxin/library/language/operations/slice.hpp"
#include "tetrodotoxin/library/language/operations/subtract.hpp"
#include "tetrodotoxin/library/language/parser/literal.hpp"
#include "ttx/concept/reference.hpp"

using namespace Perimortem::Memory;
using namespace Perimortem::Utility;
using namespace Tetrodotoxin;
using namespace Ttx::Concept;
using namespace Ttx::Lexical;

// Prefix operators keep Slice inside their operand and stop before
// multiplicative grammar, so concrete unary owners never replay either level.
static constexpr Count prefix_precedence = 31;

static auto get_precedence(Code::Type operation) -> Count {
  switch (operation) {
  case Code::Type::SliceOp:
    return 40;
  case Code::Type::DivOp:
  case Code::Type::ModOp:
  case Code::Type::MulOp:
    return 30;
  case Code::Type::SubOp:
    return 20;
  case Code::Type::LessOp:
  case Code::Type::GreaterOp:
  case Code::Type::GreaterEqOp:
  case Code::Type::LessEqOp:
    return 10;
  case Code::Type::CmpOp:
  case Code::Type::NotEqOp:
    return 5;
  default:
    return 0;
  }
}

static auto parse_primary(
    Allocator::Arena& domain,
    Library::Language::Materializations& materializations,
    Cursor& cursor,
    const Abstract& source_context) -> Option<Library::Language::Expression&> {
  if (cursor.matches(Code::Type::Addressable)) {
    Token token = cursor.consume();
    Span span(token);
    Anchor anchor = Anchor::create(token, span);

    Perimortem::Core::View::Bytes route =
        token.caculate_text(cursor.get_source_text());
    auto& identifier =
        Library::Language::Identifier::create_authored(domain, route, anchor);

    return identifier;
  }

  if (cursor.matches(Code::Type::NotOp)) {
    return Library::Language::Operations::Not::parse(
        domain, materializations, cursor, source_context);
  }

  if (cursor.matches(Code::Type::SubOp)) {
    // Literal keeps the sign for decimal and Real spellings. Every other
    // leading subtraction Token enters the general Negate grammar.
    switch (cursor.peek(1).get_code().get_type()) {
    case Code::Type::Numeric:
    case Code::Type::Float:
      break;
    default:
      return Library::Language::Operations::Negate::parse(
          domain, materializations, cursor, source_context);
    }
  }

  auto literal = Library::Language::Parser::Literal::parse(
      domain, materializations, cursor, source_context);
  if (!literal) {
    return {};
  }

  return *literal;
}

static auto parse_expression(
    Allocator::Arena& domain,
    Library::Language::Materializations& materializations,
    Cursor& cursor,
    const Abstract& source_context,
    Count minimum_precedence) -> Option<Library::Language::Expression&> {
  auto primary =
      parse_primary(domain, materializations, cursor, source_context);
  if (!primary) {
    return {};
  }

  Reference<Library::Language::Expression> expression(*primary);
  while (True) {
    // Expression owns only precedence and operator selection. Each selected
    // operator consumes its complete grammar and returns one semantic edge.
    Count precedence = get_precedence(cursor.get_code().get_type());
    if (precedence < minimum_precedence) {
      return expression.get();
    }

    switch (cursor.get_code().get_type()) {
    case Code::Type::SliceOp: {
      auto slice = Library::Language::Operations::Slice::parse(
          domain, materializations, cursor, source_context, expression.get());
      if (!slice) {
        return {};
      }

      expression = *slice;
      break;
    }
    case Code::Type::MulOp: {
      auto multiply = Library::Language::Operations::Multiply::parse(
          domain, materializations, cursor, source_context, expression.get());
      if (!multiply) {
        return {};
      }

      expression = *multiply;
      break;
    }
    case Code::Type::DivOp: {
      auto divide = Library::Language::Operations::Divide::parse(
          domain, materializations, cursor, source_context, expression.get());
      if (!divide) {
        return {};
      }

      expression = *divide;
      break;
    }
    case Code::Type::ModOp: {
      auto modulo = Library::Language::Operations::Modulo::parse(
          domain, materializations, cursor, source_context, expression.get());
      if (!modulo) {
        return {};
      }

      expression = *modulo;
      break;
    }
    case Code::Type::SubOp: {
      auto subtract = Library::Language::Operations::Subtract::parse(
          domain, materializations, cursor, source_context, expression.get());
      if (!subtract) {
        return {};
      }

      expression = *subtract;
      break;
    }
    case Code::Type::LessOp: {
      auto less = Library::Language::Operations::Less::parse(
          domain, materializations, cursor, source_context, expression.get());
      if (!less) {
        return {};
      }

      expression = *less;
      break;
    }
    case Code::Type::GreaterOp: {
      auto greater = Library::Language::Operations::Greater::parse(
          domain, materializations, cursor, source_context, expression.get());
      if (!greater) {
        return {};
      }

      expression = *greater;
      break;
    }
    case Code::Type::GreaterEqOp: {
      auto greater_equal = Library::Language::Operations::GreaterEqual::parse(
          domain, materializations, cursor, source_context, expression.get());
      if (!greater_equal) {
        return {};
      }

      expression = *greater_equal;
      break;
    }
    case Code::Type::LessEqOp: {
      auto less_equal = Library::Language::Operations::LessEqual::parse(
          domain, materializations, cursor, source_context, expression.get());
      if (!less_equal) {
        return {};
      }

      expression = *less_equal;
      break;
    }
    case Code::Type::CmpOp: {
      auto equal = Library::Language::Operations::Equal::parse(
          domain, materializations, cursor, source_context, expression.get());
      if (!equal) {
        return {};
      }

      expression = *equal;
      break;
    }
    case Code::Type::NotEqOp: {
      auto not_equal = Library::Language::Operations::NotEqual::parse(
          domain, materializations, cursor, source_context, expression.get());
      if (!not_equal) {
        return {};
      }

      expression = *not_equal;
      break;
    }
    default:
      return expression.get();
    }
  }
}

auto Library::Language::Parser::Expression::parse(
    Allocator::Arena& domain,
    Materializations& materializations,
    Cursor& cursor,
    const Abstract& source_context) -> Option<Language::Expression&> {
  auto transaction = cursor.branch();
  auto parsed = parse_expression(
      domain, materializations, transaction, source_context, 0);
  if (!parsed) {
    return {};
  }

  // Only this final join changes caller position. Nested operands and partial
  // postfix chains remain private to the transaction Cursor.
  cursor.join(transaction);
  return *parsed;
}

auto Library::Language::Parser::Expression::parse_operand(
    Allocator::Arena& domain,
    Materializations& materializations,
    Cursor& cursor,
    const Abstract& source_context,
    Code::Type operation) -> Option<Language::Expression&> {
  Count precedence = get_precedence(operation);
  if (precedence == 0) {
    return {};
  }

  Errors operand_errors;
  auto transaction = cursor.branch(operand_errors);
  auto parsed = parse_expression(
      domain, materializations, transaction, source_context, precedence + 1);
  if (!parsed) {
    return {};
  }

  cursor.join(transaction);
  return *parsed;
}

auto Library::Language::Parser::Expression::parse_prefix_operand(
    Allocator::Arena& domain,
    Materializations& materializations,
    Cursor& cursor,
    const Abstract& source_context) -> Option<Language::Expression&> {
  Errors operand_errors;
  auto transaction = cursor.branch(operand_errors);
  auto parsed = parse_expression(
      domain, materializations, transaction, source_context, prefix_precedence);
  if (!parsed) {
    return {};
  }

  cursor.join(transaction);
  return *parsed;
}
