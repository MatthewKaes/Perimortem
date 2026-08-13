// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/language/parser/expression.hpp"

#include "tetrodotoxin/library/language/access/address.hpp"
#include "tetrodotoxin/library/language/access/call.hpp"
#include "tetrodotoxin/library/language/access/index.hpp"
#include "tetrodotoxin/library/language/access/slice.hpp"
#include "tetrodotoxin/library/language/access/swizzle.hpp"
#include "tetrodotoxin/library/language/access/type.hpp"
#include "tetrodotoxin/library/language/expressions/identifier.hpp"
#include "tetrodotoxin/library/language/model/parser/pack.hpp"
#include "tetrodotoxin/library/language/operations/add.hpp"
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
#include "tetrodotoxin/library/language/operations/range.hpp"
#include "tetrodotoxin/library/language/operations/subtract.hpp"
#include "tetrodotoxin/library/language/parser/literal.hpp"
#include "ttx/concept/reference.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Tetrodotoxin;
using namespace Ttx::Concept;
using namespace Ttx::Lexical;

// Prefix operators keep Slice access inside their operand and stop before
// multiplicative grammar, so concrete unary owners never replay either level.
static constexpr Count prefix_precedence = 31;

using ReceiverParser = auto (*)(
    Allocator::Arena&,
    Library::Language::Monograph&,
    Cursor&,
    Library::Language::Expression&) -> Option<Library::Language::Expression&>;

using BinaryParser = auto (*)(
    Allocator::Arena&,
    Library::Language::Monograph&,
    Cursor&,
    Library::Language::Model::Pack&,
    Span) -> Option<Library::Language::Expression&>;

class Parsed {
 public:
  constexpr Parsed(Library::Language::Model::Pack& pack, Span span)
      : pack(pack), span(span) {}

  Reference<Library::Language::Model::Pack> pack;
  Span span;
};

struct BinaryRule {
  Count precedence;
  BinaryParser parse;
};

static auto find_postfix(Code::Type code) -> Option<ReceiverParser> {
  switch (code) {
  case Code::Type::AddressOp:
    return &Library::Language::Access::Address::parse;
  case Code::Type::CallOp:
    return &Library::Language::Access::Call::parse;
  case Code::Type::BracketStart:
    return &Library::Language::Access::Index::parse;
  case Code::Type::TypeAccessOp:
    return &Library::Language::Access::Type::parse;
  case Code::Type::ValueAccessOp:
    return &Library::Language::Access::Slice::parse;
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
  case Code::Type::AddOp:
    return BinaryRule{20, &Library::Language::Operations::Add::parse};
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
  case Code::Type::RangeOp:
    return BinaryRule{1, &Library::Language::Operations::Range::parse};
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
    Library::Language::Monograph& source,
    Cursor& cursor) -> Option<Library::Language::Model::Pack&> {
  if (cursor.matches(Code::Type::PackingStart)) {
    return Library::Language::Model::Parser::Pack::parse(
        domain, source, cursor, True);
  }

  if (cursor.matches(Code::Type::Type) ||
      cursor.matches(Code::Type::Addressable) ||
      cursor.matches(Code::Type::Self) || cursor.matches(Code::Type::Source)) {
    Token token = cursor.consume();
    return Library::Language::Expressions::Identifier::create_authored(
        domain, token, cursor.get_source_text(), Anchor::create(Span(token)));
  }

  if (cursor.matches(Code::Type::NotOp)) {
    auto operation =
        Library::Language::Operations::Not::parse(domain, source, cursor);
    BAIL_IF(!operation);
    return static_cast<Library::Language::Model::Pack&>(*operation);
  }

  if (cursor.matches(Code::Type::SubOp)) {
    // Literal keeps the sign for decimal and Real spellings. Every other
    // leading subtraction Token enters the general Negate grammar.
    switch (cursor.peek(1).get_code().get_type()) {
    case Code::Type::Numeric:
    case Code::Type::Float:
      break;
    default:
      auto operation =
          Library::Language::Operations::Negate::parse(domain, source, cursor);
      BAIL_IF(!operation);
      return static_cast<Library::Language::Model::Pack&>(*operation);
    }
  }

  auto literal =
      Library::Language::Parser::Literal::parse(domain, source, cursor);
  BAIL_IF(!literal);

  return *literal;
}

static auto parse_expression(
    Allocator::Arena& domain,
    Library::Language::Monograph& source,
    Cursor& cursor,
    Count minimum_precedence) -> Option<Parsed> {
  Token start = cursor.current();
  auto primary = parse_primary(domain, source, cursor);
  BAIL_IF(!primary);

  Parsed parsed(*primary, Span(start, cursor.peek(-1)));

  // Postfix Access binds to the complete receiver before binary grammar. Each
  // completed node becomes the receiver for the next suffix. The binary loop
  // below then resolves that finished chain as its left Expression.
  while (True) {
    if (cursor.matches(Code::Type::SwizzleOp)) {
      auto selected = Library::Language::Access::Swizzle::parse(
          domain, source, cursor, parsed.pack.get(), parsed.span);
      BAIL_IF(!selected);

      parsed = Parsed(*selected, Span(start, cursor.peek(-1)));
      continue;
    }

    auto postfix = find_postfix(cursor.get_code().get_type());
    if (!postfix) {
      break;
    }

    auto receiver = parsed.pack.get().select<Library::Language::Expression>();
    if (!receiver) {
      cursor.create_expression_error(
          Anchor::create(cursor.current(), parsed.span, Span(cursor.current())),
          "Library postfix access requires one Expression receiver Pack."_view,
          "Select through one unlabelled scalar value; named and multi-value "
          "Packs have no implicit receiver."_view);
      return {};
    }

    auto selected = postfix.visit(
        []() -> Option<Library::Language::Expression&> { return {}; },
        [&](ReceiverParser parser) {
          return parser(domain, source, cursor, *receiver);
        });
    BAIL_IF(!selected);

    parsed = Parsed(*selected, Span(start, cursor.peek(-1)));
  }

  while (True) {
    auto binary = find_binary(cursor.get_code().get_type());
    Bool applicable = binary.visit(
        []() -> Bool { return False; },
        [minimum_precedence](const BinaryRule& selected) -> Bool {
          return Bool(selected.precedence >= minimum_precedence);
        });
    if (!applicable) {
      return parsed;
    }

    auto selected = binary.visit(
        []() -> Option<Library::Language::Expression&> { return {}; },
        [&](const BinaryRule& selected) {
          return selected.parse(
              domain, source, cursor, parsed.pack.get(), parsed.span);
        });
    BAIL_IF(!selected);

    parsed = Parsed(*selected, Span(start, cursor.peek(-1)));
  }
}

auto Library::Language::Parser::Expression::parse(
    Allocator::Arena& domain,
    Language::Monograph& source,
    Cursor& cursor) -> Option<Language::Model::Pack&> {
  auto transaction = cursor.branch();
  auto parsed = parse_expression(domain, source, transaction, 0);
  BAIL_IF(!parsed);

  // Only this final join changes caller position. Nested operands and partial
  // postfix chains remain private to the transaction Cursor.
  cursor.join(transaction);
  return parsed->pack.get();
}

auto Library::Language::Parser::Expression::parse_operand(
    Allocator::Arena& domain,
    Language::Monograph& source,
    Cursor& cursor,
    Code::Type operation) -> Option<Language::Model::Pack&> {
  Count precedence = get_precedence(operation);
  BAIL_IF(precedence == 0);

  Errors operand_errors;
  auto transaction = cursor.branch(operand_errors);
  auto parsed = parse_expression(domain, source, transaction, precedence + 1);
  BAIL_IF(!parsed);

  cursor.join(transaction);
  return parsed->pack.get();
}

auto Library::Language::Parser::Expression::parse_prefix_operand(
    Allocator::Arena& domain,
    Language::Monograph& source,
    Cursor& cursor) -> Option<Language::Model::Pack&> {
  Errors operand_errors;
  auto transaction = cursor.branch(operand_errors);
  auto parsed =
      parse_expression(domain, source, transaction, prefix_precedence);
  BAIL_IF(!parsed);

  cursor.join(transaction);
  return parsed->pack.get();
}
