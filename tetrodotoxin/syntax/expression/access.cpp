// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/syntax/expression/access.hpp"

#include "perimortem/memory/managed/vector.hpp"

#include "tetrodotoxin/syntax/context.hpp"
#include "tetrodotoxin/syntax/pack.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Tetrodotoxin::Syntax;
using namespace Tetrodotoxin::Lexical;

static auto parse_field(Context& ctx, Expression::Expression* base)
    -> Expression::Expression* {
  ctx.advance();
  View::Bytes name = ctx.get_current().get_text();
  ctx.advance();
  return Expression::Expression::construct<Expression::FieldAccess>(
      ctx, base, name);
}

static auto parse_call(Context& ctx, Expression::Expression* base)
    -> Expression::Expression* {
  ctx.advance();
  View::Bytes name;

  switch (ctx.get_current().get_class().get_type()) {
  case Class::Type::Addressable:
  case Class::Type::Type:
  case Class::Type::New:
    name = ctx.get_current().get_text();
    ctx.advance();
    break;

  default:
    ctx.error("Expected a method name after '->'"_view);
    break;
  }

  return Expression::Expression::construct<Expression::Call>(
      ctx, base, name, Pack::parse(ctx));
}

static auto parse_index(Context& ctx, Expression::Expression* base)
    -> Expression::Expression* {
  ctx.advance();
  Expression::Expression* index = Expression::Expression::parse(ctx);

  switch (ctx.get_current().get_class().get_type()) {
  case Class::Type::SliceOp: {
    ctx.advance();
    Expression::Expression* count = Expression::Expression::parse(ctx);
    ctx.require(Class::Type::IndexEnd);
    return Expression::Expression::construct<Expression::IndexSlice>(
        ctx, base, index, count);
  }

  default:
    break;
  }

  ctx.require(Class::Type::IndexEnd);
  return Expression::Expression::construct<Expression::Index>(ctx, base, index);
}

static auto parse_swizzle(Context& ctx, Expression::Expression* base)
    -> Expression::Expression* {
  ctx.advance();
  Managed::Vector<Expression::Expression*> parts(ctx.get_arena());

  while (!ctx.matches(Class::Type::IndexEnd) && !ctx.is_done()) {
    parts.insert(Expression::Expression::parse(ctx));

    switch (ctx.get_current().get_class().get_type()) {
    case Class::Type::SliceOp: {
      ctx.advance();
      Expression::Expression* count = Expression::Expression::parse(ctx);
      ctx.require(Class::Type::IndexEnd);
      return Expression::Expression::construct<Expression::Slice>(
          ctx, base, parts[0], count);
    }

    case Class::Type::PackingOp:
      ctx.advance();
      break;

    default:
      ctx.require(Class::Type::IndexEnd);
      return Expression::Expression::construct<Expression::Swizzle>(
          ctx, base, parts.get_view());
    }
  }

  ctx.require(Class::Type::IndexEnd);
  return Expression::Expression::construct<Expression::Swizzle>(
      ctx, base, parts.get_view());
}

auto Expression::parse_access_chain(Context& ctx, Expression* base)
    -> Expression* {
  while (True) {
    switch (ctx.get_current().get_class().get_type()) {
    case Class::Type::AddressOp:
      switch (ctx.get_peek().get_class().get_type()) {
      case Class::Type::Addressable:
      case Class::Type::Type:
        base = parse_field(ctx, base);
        continue;

      default:
        return base;
      }

    case Class::Type::CallOp:
      base = parse_call(ctx, base);
      continue;

    case Class::Type::IndexStart:
      base = parse_index(ctx, base);
      continue;

    case Class::Type::SwizzleOp:
      base = parse_swizzle(ctx, base);
      continue;

    case Class::Type::PackingStart:
      if (base && base->get_class() == Class::Type::Type) {
        ctx.error(
            "Types are not callable"_view,
            "Use explicit dispatch such as 'Type -> from(...)' or 'Type -> new(...)'."_view);
        ctx.recover_to_matching(
            Class::Type::PackingStart, Class::Type::PackingEnd);
        continue;
      }
      return base;

    default:
      return base;
    }
  }
}
