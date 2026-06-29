// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/syntax/ast/param.hpp"

#include "perimortem/memory/managed/vector.hpp"

#include "tetrodotoxin/syntax/context.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Tetrodotoxin::Syntax;
using namespace Tetrodotoxin::Lexical;

auto Ast::Param::parse(Context& ctx) -> Param {
  Param result;
  Managed::Vector<Attribute> attrs(ctx.get_arena());

  while (ctx.matches(Class::Type::Attribute)) {
    attrs.insert(Attribute::parse(ctx));
  }

  result.attributes = attrs.get_view();

  if (ctx.matches(Class::Type::AddressOp)) {
    ctx.advance();
  }

  if (!ctx.matches(Class::Type::Addressable)) {
    ctx.error(
        "Expected a parameter name"_view,
        "Parameter names are lowercase, e.g. '.source : View[Bytes]'"_view);
    return result;
  }

  result.name = ctx.get_current().get_text();
  ctx.advance();

  ctx.require(Class::Type::Define);
  result.type = Type::Ref::parse(ctx);
  return result;
}
