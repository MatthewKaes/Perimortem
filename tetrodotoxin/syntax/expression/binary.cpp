// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/syntax/expression/binary.hpp"

#include "tetrodotoxin/syntax/context.hpp"
#include "tetrodotoxin/syntax/expression/access.hpp"
#include "tetrodotoxin/syntax/expression/primary.hpp"
#include "tetrodotoxin/syntax/expression/unary.hpp"

using namespace Perimortem::Core;
using namespace Tetrodotoxin::Syntax;
using namespace Tetrodotoxin::Lexical;

static auto is_cmp_op(Class::Type type) -> Bool {
  switch (type) {
  case Class::Type::CmpOp:
  case Class::Type::NotEqOp:
  case Class::Type::LessOp:
  case Class::Type::GreaterOp:
  case Class::Type::LessEqOp:
  case Class::Type::GreaterEqOp:
    return True;

  default:
    return False;
  }
}

static auto parse_unary(Context& ctx) -> Expression::Expression* {
  switch (ctx.get_current().get_class().get_type()) {
  case Class::Type::NotOp: {
    ctx.advance();
    return Expression::Expression::construct<Expression::Unary>(
        ctx, Class::Type::NotOp, parse_unary(ctx));
  }

  case Class::Type::SubOp: {
    ctx.advance();
    return Expression::Expression::construct<Expression::Unary>(
        ctx, Class::Type::SubOp, parse_unary(ctx));
  }

  default:
    return Expression::parse_access_chain(ctx, Expression::parse_primary(ctx));
  }
}

using ParseExpr = Expression::Expression* (*)(Context&);
using OpMatcher = Bool (*)(Context&, Class::Type);

static auto parse_binary_chain(
    Context& ctx,
    ParseExpr parse_operand,
    OpMatcher matches_op) -> Expression::Expression* {
  Expression::Expression* left_expression = parse_operand(ctx);

  while (True) {
    Class::Type operator_type = ctx.get_current().get_class().get_type();

    if (!matches_op(ctx, operator_type)) {
      return left_expression;
    }

    ctx.advance();
    Expression::Expression* node =
        Expression::Expression::construct<Expression::Binary>(
            ctx, operator_type, left_expression, parse_operand(ctx));
    left_expression = node;
  }
}

static auto parse_binary_once(
    Context& ctx,
    ParseExpr parse_operand,
    OpMatcher matches_op,
    Class::Type klass) -> Expression::Expression* {
  Expression::Expression* left_expression = parse_operand(ctx);
  Class::Type operator_type = ctx.get_current().get_class().get_type();

  if (!matches_op(ctx, operator_type)) {
    return left_expression;
  }

  ctx.advance();
  return Expression::Expression::construct<Expression::Binary>(
      ctx, klass == Class::Type::EndOfStream ? operator_type : klass,
      left_expression, parse_operand(ctx));
}

static auto matches_mul_op(Context&, Class::Type operator_type) -> Bool {
  switch (operator_type) {
  case Class::Type::MulOp:
  case Class::Type::DivOp:
  case Class::Type::ModOp:
    return True;

  default:
    return False;
  }
}

static auto matches_add_op(Context& ctx, Class::Type operator_type) -> Bool {
  switch (operator_type) {
  case Class::Type::AddOp:
    return ctx.get_peek().get_class() != Class::Type::Define;

  case Class::Type::SubOp:
    return True;

  default:
    return False;
  }
}

static auto matches_cmp_op(Context&, Class::Type operator_type) -> Bool {
  return is_cmp_op(operator_type);
}

static auto matches_and_op(Context&, Class::Type operator_type) -> Bool {
  return operator_type == Class::Type::And;
}

static auto matches_or_op(Context&, Class::Type operator_type) -> Bool {
  return operator_type == Class::Type::Or;
}

static auto matches_range_op(Context&, Class::Type operator_type) -> Bool {
  return operator_type == Class::Type::RangeOp;
}

static auto parse_mul(Context& ctx) -> Expression::Expression* {
  return parse_binary_chain(ctx, parse_unary, matches_mul_op);
}

static auto parse_add(Context& ctx) -> Expression::Expression* {
  return parse_binary_chain(ctx, parse_mul, matches_add_op);
}

static auto parse_cmp(Context& ctx) -> Expression::Expression* {
  return parse_binary_once(
      ctx, parse_add, matches_cmp_op, Class::Type::EndOfStream);
}

static auto parse_and(Context& ctx) -> Expression::Expression* {
  return parse_binary_chain(ctx, parse_cmp, matches_and_op);
}

static auto parse_or(Context& ctx) -> Expression::Expression* {
  return parse_binary_chain(ctx, parse_and, matches_or_op);
}

static auto parse_range(Context& ctx) -> Expression::Expression* {
  return parse_binary_once(
      ctx, parse_or, matches_range_op, Class::Type::RangeOp);
}

auto Expression::Binary::parse(Context& ctx) -> Expression* {
  return parse_range(ctx);
}

auto Expression::Binary::precedence(Class klass) -> Count {
  switch (klass.get_type()) {
  case Class::Type::MulOp:
  case Class::Type::DivOp:
  case Class::Type::ModOp:
    return 4;

  case Class::Type::AddOp:
  case Class::Type::SubOp:
    return 3;

  case Class::Type::CmpOp:
  case Class::Type::NotEqOp:
  case Class::Type::LessOp:
  case Class::Type::GreaterOp:
  case Class::Type::LessEqOp:
  case Class::Type::GreaterEqOp:
    return 2;

  case Class::Type::And:
    return 1;

  case Class::Type::Or:
    return 0;

  default:
    return 5;
  }
}
