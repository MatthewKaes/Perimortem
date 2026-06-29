// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/syntax/ast/attribute.hpp"

#include "tetrodotoxin/syntax/context.hpp"

using namespace Perimortem::Core;
using namespace Tetrodotoxin::Syntax;
using namespace Tetrodotoxin::Lexical;

auto Ast::Attribute::parse(Context& ctx) -> Attribute {
  Attribute result;

  if (!ctx.expect(Class::Type::Attribute)) {
    return result;
  }

  // Token text is "@name" so we need to strip the leading '@'.
  result.name = ctx.get_current().get_text().slice(1);
  ctx.advance();

  // Attributes take a Pack similar to functions
  // @name(.key = val, ...)
  if (ctx.matches(Class::Type::PackingStart)) {
    result.pack = Pack::parse(ctx);
  }

  return result;
}
