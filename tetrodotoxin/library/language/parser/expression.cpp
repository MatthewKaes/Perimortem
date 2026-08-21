// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/language/parser/expression.hpp"

#include "tetrodotoxin/library/language/access/address.hpp"
#include "tetrodotoxin/library/language/access/call.hpp"
#include "tetrodotoxin/library/language/access/index.hpp"
#include "tetrodotoxin/library/language/access/propagate.hpp"
#include "tetrodotoxin/library/language/access/slice.hpp"
#include "tetrodotoxin/library/language/access/swizzle.hpp"
#include "tetrodotoxin/library/language/access/type.hpp"
#include "tetrodotoxin/library/language/access/unwrap.hpp"
#include "tetrodotoxin/library/language/expressions/identifier.hpp"
#include "tetrodotoxin/library/language/expressions/initializer.hpp"
#include "tetrodotoxin/library/language/model/parser/pack.hpp"
#include "tetrodotoxin/library/language/operations/add.hpp"
#include "tetrodotoxin/library/language/operations/add_assignment.hpp"
#include "tetrodotoxin/library/language/operations/and.hpp"
#include "tetrodotoxin/library/language/operations/assignment.hpp"
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
#include "tetrodotoxin/library/language/operations/subtract_assignment.hpp"
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

static auto associate_expression(
    Cursor& cursor,
    Library::Language::Model::Pack& pack) -> void {
  auto expression = pack.select<Library::Language::Expression>();
  if (!expression || !expression->get_anchor()) {
    return;
  }
  cursor.get_associations().create(*expression->get_anchor(), *expression);
}

static auto is_postfix(Code::Type code) -> Bool {
  switch (code) {
  case Code::Type::AddressOp:
  case Code::Type::CallOp:
  case Code::Type::BracketStart:
  case Code::Type::TypeAccessOp:
  case Code::Type::ValueAccessOp:
  case Code::Type::NotOp:
  case Code::Type::QuestionOp:
    return True;
  default:
    return False;
  }
}

static auto parse_postfix(
    Code::Type code,
    const Abstract& context,
    Cursor& cursor,
    Library::Language::Expression& receiver)
    -> Option<Library::Language::Expression&> {
  switch (code) {
  case Code::Type::AddressOp:
    return Library::Language::Access::Address::parse(context, cursor, receiver);
  case Code::Type::CallOp:
    return Library::Language::Access::Call::parse(context, cursor, receiver);
  case Code::Type::BracketStart:
    return Library::Language::Access::Index::parse(context, cursor, receiver);
  case Code::Type::TypeAccessOp:
    return Library::Language::Access::Type::parse(context, cursor, receiver);
  case Code::Type::ValueAccessOp:
    return Library::Language::Access::Slice::parse(context, cursor, receiver);
  case Code::Type::NotOp:
    return Library::Language::Access::Unwrap::parse(context, cursor, receiver);
  case Code::Type::QuestionOp:
    return Library::Language::Access::Propagate::parse(
        context, cursor, receiver);
  default:
    return {};
  }
}

static auto get_precedence(Code::Type code) -> Count {
  switch (code) {
  case Code::Type::DivOp:
  case Code::Type::ModOp:
  case Code::Type::MulOp:
    return 30;
  case Code::Type::AddOp:
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
  case Code::Type::And:
    return 4;
  case Code::Type::Or:
    return 3;
  case Code::Type::RangeOp:
    return 2;
  case Code::Type::Assign:
  case Code::Type::AddAssign:
  case Code::Type::SubAssign:
    return 1;
  default:
    return 0;
  }
}

static auto parse_binary(
    Code::Type code,
    const Abstract& context,
    Cursor& cursor,
    Library::Language::Model::Pack& left,
    Span left_span) -> Option<Library::Language::Expression&> {
  switch (code) {
  case Code::Type::DivOp:
    return Library::Language::Operations::Divide::parse(
        context, cursor, left, left_span);
  case Code::Type::ModOp:
    return Library::Language::Operations::Modulo::parse(
        context, cursor, left, left_span);
  case Code::Type::MulOp:
    return Library::Language::Operations::Multiply::parse(
        context, cursor, left, left_span);
  case Code::Type::AddOp:
    return Library::Language::Operations::Add::parse(
        context, cursor, left, left_span);
  case Code::Type::SubOp:
    return Library::Language::Operations::Subtract::parse(
        context, cursor, left, left_span);
  case Code::Type::LessOp:
    return Library::Language::Operations::Less::parse(
        context, cursor, left, left_span);
  case Code::Type::GreaterOp:
    return Library::Language::Operations::Greater::parse(
        context, cursor, left, left_span);
  case Code::Type::GreaterEqOp:
    return Library::Language::Operations::GreaterEqual::parse(
        context, cursor, left, left_span);
  case Code::Type::LessEqOp:
    return Library::Language::Operations::LessEqual::parse(
        context, cursor, left, left_span);
  case Code::Type::CmpOp:
    return Library::Language::Operations::Equal::parse(
        context, cursor, left, left_span);
  case Code::Type::NotEqOp:
    return Library::Language::Operations::NotEqual::parse(
        context, cursor, left, left_span);
  case Code::Type::And:
    return Library::Language::Operations::And::parse(
        context, cursor, left, left_span);
  case Code::Type::Or:
    return Library::Language::Operations::Or::parse(
        context, cursor, left, left_span);
  case Code::Type::RangeOp:
    return Library::Language::Operations::Range::parse(
        context, cursor, left, left_span);
  case Code::Type::Assign:
    return Library::Language::Operations::Assignment::parse(
        context, cursor, left, left_span);
  case Code::Type::AddAssign:
    return Library::Language::Operations::AddAssignment::parse(
        context, cursor, left, left_span);
  case Code::Type::SubAssign:
    return Library::Language::Operations::SubtractAssignment::parse(
        context, cursor, left, left_span);
  default:
    return {};
  }
}

static auto parse_primary(const Abstract& context, Cursor& cursor)
    -> Option<Library::Language::Model::Pack&> {
  if (cursor.matches(Code::Type::PackingStart)) {
    return Library::Language::Model::Parser::Pack::parse(context, cursor, True);
  }

  if (Library::Language::Expressions::Initializer::is_next(cursor)) {
    auto initializer =
        Library::Language::Expressions::Initializer::parse(context, cursor);
    BAIL_IF(!initializer);
    return static_cast<Library::Language::Model::Pack&>(*initializer);
  }

  if (cursor.matches(Code::Type::Type) ||
      cursor.matches(Code::Type::Addressable) ||
      cursor.matches(Code::Type::Self) || cursor.matches(Code::Type::Source)) {
    Token token = cursor.consume();
    return Library::Language::Expressions::Identifier::create_authored(
        cursor, token, Anchor::create(Span(token)));
  }

  if (cursor.matches(Code::Type::NotOp)) {
    auto operation = Library::Language::Operations::Not::parse(context, cursor);
    BAIL_IF(!operation);
    return static_cast<Library::Language::Model::Pack&>(*operation);
  }

  if (cursor.matches(Code::Type::SubOp)) {
    // Literal keeps the sign for decimal and Real spellings. Every other
    // leading subtraction Token enters the general Negate grammar.
    switch (cursor.peek(1).get_code().get_type()) {
    case Code::Type::Numeric:
    case Code::Type::Float: {
      auto literal = Library::Language::Parser::Literal::parse(context, cursor);
      BAIL_IF(!literal);
      return *literal;
    }
    default:
      auto operation =
          Library::Language::Operations::Negate::parse(context, cursor);
      BAIL_IF(!operation);
      return static_cast<Library::Language::Model::Pack&>(*operation);
    }
  }

  switch (cursor.get_code().get_type()) {
  case Code::Type::String:
  case Code::Type::Bytes:
  case Code::Type::True:
  case Code::Type::False:
  case Code::Type::Numeric:
  case Code::Type::Hex:
  case Code::Type::Float:
  case Code::Type::Embedded: {
    auto literal = Library::Language::Parser::Literal::parse(context, cursor);
    BAIL_IF(!literal);
    return *literal;
  }
  default:
    return {};
  }
}

static auto parse_expression(
    const Abstract& context,
    Cursor& cursor,
    Count minimum_precedence) -> Option<Library::Language::Model::Pack&> {
  Token start = cursor.current();
  auto primary = parse_primary(context, cursor);
  BAIL_IF(!primary);
  associate_expression(cursor, *primary);

  Reference<Library::Language::Model::Pack> parsed(*primary);
  Span parsed_span(start, cursor.peek(-1));

  // Postfix Access binds to the complete receiver before binary grammar. Each
  // completed node becomes the receiver for the next suffix. The binary loop
  // below then resolves that finished chain as its left Expression.
  while (True) {
    if (cursor.matches(Code::Type::SwizzleOp)) {
      auto selected = Library::Language::Access::Swizzle::parse(
          context, cursor, parsed.get(), parsed_span);
      BAIL_IF(!selected);

      parsed = Reference<Library::Language::Model::Pack>(*selected);
      parsed_span = Span(start, cursor.peek(-1));
      associate_expression(cursor, parsed.get());
      continue;
    }

    Code::Type postfix = cursor.get_code().get_type();
    if (!is_postfix(postfix)) {
      break;
    }

    auto receiver = parsed.get().select<Library::Language::Expression>();
    if (!receiver) {
      cursor.create_expression_error(
          Anchor::create(cursor.current(), parsed_span, Span(cursor.current())),
          "Library postfix access requires one Expression receiver Pack."_view,
          "Select through one unlabelled scalar value; named and multi-value "
          "Packs have no implicit receiver."_view);
      return {};
    }

    auto selected = parse_postfix(postfix, context, cursor, *receiver);
    BAIL_IF(!selected);

    parsed = Reference<Library::Language::Model::Pack>(*selected);
    parsed_span = Span(start, cursor.peek(-1));
    associate_expression(cursor, parsed.get());
  }

  while (True) {
    Code::Type binary = cursor.get_code().get_type();
    Count precedence = get_precedence(binary);
    if (precedence == 0 || precedence < minimum_precedence) {
      return parsed.get();
    }

    auto selected =
        parse_binary(binary, context, cursor, parsed.get(), parsed_span);
    BAIL_IF(!selected);

    parsed = Reference<Library::Language::Model::Pack>(*selected);
    parsed_span = Span(start, cursor.peek(-1));
    associate_expression(cursor, parsed.get());
  }
}

auto Library::Language::Parser::Expression::parse(
    const Abstract& context,
    Cursor& cursor) -> Option<Language::Model::Pack&> {
  Count error_count = cursor.get_error_count();
  auto parsed = parse_expression(context, cursor, 0);
  if (!parsed) {
    if (cursor.get_error_count() == error_count) {
      cursor.create_token_error(
          "Library expression requires one value operand."_view,
          "Write one literal, name, grouped Pack, or prefix expression."_view);
    }
    return {};
  }
  return *parsed;
}

auto Library::Language::Parser::Expression::parse_operand(
    const Abstract& context,
    Cursor& cursor,
    Code::Type operation) -> Option<Language::Model::Pack&> {
  Count precedence = get_precedence(operation);
  BAIL_IF(precedence == 0);

  return parse_expression(context, cursor, precedence + 1);
}

auto Library::Language::Parser::Expression::parse_write_operand(
    const Abstract& context,
    Cursor& cursor) -> Option<Language::Model::Pack&> {
  return parse_expression(context, cursor, get_precedence(Code::Type::Assign));
}

auto Library::Language::Parser::Expression::parse_prefix_operand(
    const Abstract& context,
    Cursor& cursor) -> Option<Language::Model::Pack&> {
  return parse_expression(context, cursor, prefix_precedence);
}
