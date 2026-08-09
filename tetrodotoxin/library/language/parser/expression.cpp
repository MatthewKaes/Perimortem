// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/language/parser/expression.hpp"

#include "tetrodotoxin/library/language/access/address.hpp"
#include "tetrodotoxin/library/language/access/value.hpp"
#include "tetrodotoxin/library/language/identifier.hpp"
#include "tetrodotoxin/library/language/operations/and.hpp"
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
#include "tetrodotoxin/library/language/operations/or.hpp"
#include "tetrodotoxin/library/language/operations/subtract.hpp"
#include "tetrodotoxin/library/language/parser/literal.hpp"
#include "ttx/concept/reference.hpp"

using namespace Perimortem::Memory;
using namespace Perimortem::Utility;
using namespace Tetrodotoxin;
using namespace Ttx::Concept;
using namespace Ttx::Lexical;

// Prefix operators keep Value access inside their operand and stop before
// multiplicative grammar, so concrete unary owners never replay either level.
static constexpr Count prefix_precedence = 31;

using ReceiverParser = auto (*)(
    Allocator::Arena&,
    Library::Language::Materializations&,
    Cursor&,
    const Abstract&,
    Library::Language::Expression&) -> Option<Library::Language::Expression&>;

struct BinaryRule {
  Count precedence;
  ReceiverParser parse;
};

static auto find_postfix(Code::Type code) -> Option<ReceiverParser> {
  switch (code) {
  case Code::Type::AddressOp:
    return &Library::Language::Access::Address::parse;
  case Code::Type::ValueAccessOp:
    return &Library::Language::Access::Value::parse;
  default:
    return {};
  }
}

static auto find_binary(Code::Type code) -> Option<BinaryRule> {
  switch (code) {
  case Code::Type::DivOp:
    return BinaryRule{30, &Library::Language::Operations::Divide::parse};
  case Code::Type::ModOp:
    return BinaryRule{30, &Library::Language::Operations::Modulo::parse};
  case Code::Type::MulOp:
    return BinaryRule{30, &Library::Language::Operations::Multiply::parse};
  case Code::Type::SubOp:
    return BinaryRule{20, &Library::Language::Operations::Subtract::parse};
  case Code::Type::LessOp:
    return BinaryRule{10, &Library::Language::Operations::Less::parse};
  case Code::Type::GreaterOp:
    return BinaryRule{10, &Library::Language::Operations::Greater::parse};
  case Code::Type::GreaterEqOp:
    return BinaryRule{10, &Library::Language::Operations::GreaterEqual::parse};
  case Code::Type::LessEqOp:
    return BinaryRule{10, &Library::Language::Operations::LessEqual::parse};
  case Code::Type::CmpOp:
    return BinaryRule{5, &Library::Language::Operations::Equal::parse};
  case Code::Type::NotEqOp:
    return BinaryRule{5, &Library::Language::Operations::NotEqual::parse};
  case Code::Type::AndOp:
    return BinaryRule{3, &Library::Language::Operations::And::parse};
  case Code::Type::OrOp:
    return BinaryRule{2, &Library::Language::Operations::Or::parse};
  default:
    return {};
  }
}

static auto get_precedence(Code::Type operation) -> Count {
  return find_binary(operation).visit(
      []() { return Count(0); },
      [](const BinaryRule& selected) { return selected.precedence; });
}

static auto parse_primary(
    Allocator::Arena& domain,
    Library::Language::Materializations& materializations,
    Cursor& cursor,
    const Abstract& source_context) -> Option<Library::Language::Expression&> {
  if (cursor.matches(Code::Type::Addressable) ||
      cursor.matches(Code::Type::Self)) {
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

  // Postfix Access binds to the complete receiver before binary grammar. Each
  // completed node becomes the receiver for the next suffix. The binary loop
  // below then resolves that finished chain as its left Expression.
  while (True) {
    auto postfix = find_postfix(cursor.get_code().get_type());
    if (!postfix) {
      break;
    }

    auto parsed = postfix.visit(
        []() -> Option<Library::Language::Expression&> { return {}; },
        [&](ReceiverParser selected) {
          return selected(
              domain, materializations, cursor, source_context,
              expression.get());
        });
    if (!parsed) {
      return {};
    }

    expression = *parsed;
  }

  while (True) {
    auto binary = find_binary(cursor.get_code().get_type());
    Bool applicable = binary.visit(
        []() -> Bool { return False; },
        [minimum_precedence](const BinaryRule& selected) -> Bool {
          return Bool(selected.precedence >= minimum_precedence);
        });
    if (!applicable) {
      return expression.get();
    }

    auto parsed = binary.visit(
        []() -> Option<Library::Language::Expression&> { return {}; },
        [&](const BinaryRule& selected) {
          return selected.parse(
              domain, materializations, cursor, source_context,
              expression.get());
        });
    if (!parsed) {
      return {};
    }

    expression = *parsed;
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
