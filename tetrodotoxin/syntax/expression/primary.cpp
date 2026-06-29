// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/syntax/expression/primary.hpp"

#include "perimortem/memory/managed/vector.hpp"

#include "tetrodotoxin/syntax/context.hpp"
#include "tetrodotoxin/syntax/expression/addressable.hpp"
#include "tetrodotoxin/syntax/expression/data.hpp"
#include "tetrodotoxin/syntax/expression/literal.hpp"
#include "tetrodotoxin/syntax/expression/type_access.hpp"
#include "tetrodotoxin/syntax/pack.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Tetrodotoxin::Syntax;
using namespace Tetrodotoxin::Lexical;

static auto parse_literal(Context& ctx) -> Expression::Expression* {
  Expression::Expression* node =
      Expression::Expression::construct<Expression::Literal>(
          ctx, ctx.get_current().get_class(), ctx.get_current().get_text());
  ctx.advance();
  return node;
}

static auto parse_addressable(Context& ctx) -> Expression::Expression* {
  Expression::Expression* node =
      Expression::Expression::construct<Expression::Addressable>(
          ctx, ctx.get_current().get_class(), ctx.get_current().get_text());
  ctx.advance();
  return node;
}

static auto parse_array(Context& ctx) -> Expression::Expression* {
  ctx.require(Class::Type::ScopeStart);
  Managed::Vector<Expression::Expression*> elems(ctx.get_arena());

  while (!ctx.matches(Class::Type::ScopeEnd) && !ctx.is_done()) {
    elems.insert(Expression::Expression::parse(ctx));

    if (!ctx.matches(Class::Type::PackingOp)) {
      break;
    }

    ctx.advance();
  }

  ctx.require(Class::Type::ScopeEnd);
  return Expression::Expression::construct<Expression::Data>(
      ctx, elems.get_view());
}

auto Expression::parse_primary(Context& ctx) -> Expression* {
  switch (ctx.get_current().get_class().get_type()) {
  case Class::Type::Numeric:
  case Class::Type::Float:
  case Class::Type::String:
  case Class::Type::Bytes:
  case Class::Type::Embedded:
  case Class::Type::True:
  case Class::Type::False:
    return parse_literal(ctx);

  case Class::Type::Addressable:
  case Class::Type::Self:
  case Class::Type::Package:
  case Class::Type::Discard:
    return parse_addressable(ctx);

  case Class::Type::Type: {
    Ref ref = Ref::parse(ctx);
    return Expression::construct<TypeAccess>(ctx, ref);
  }

  case Class::Type::PackingStart:
    return Pack::parse(ctx);

  case Class::Type::ScopeStart:
    return parse_array(ctx);

  default:
    ctx.error("Unexpected token in expression"_view);
    Expression* err = Expression::make(ctx, Class::Type::Unknown);
    ctx.advance();
    return err;
  }
}
