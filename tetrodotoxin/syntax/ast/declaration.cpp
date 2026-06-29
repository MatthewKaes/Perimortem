// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/syntax/ast/declaration.hpp"

#include "perimortem/memory/managed/vector.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Tetrodotoxin::Lexical;
using namespace Tetrodotoxin::Syntax;

auto Ast::DeclarationPrefix::parse(Context& ctx) -> DeclarationPrefix {
  DeclarationPrefix result;
  result.documentation = Comment::parse(ctx);

  // `/>` wraps the following declaration as @if(false) { ... }.
  if (ctx.matches(Class::Type::Disabled)) {
    result.disabled = True;
    ctx.advance();
  }

  Managed::Vector<Attribute> attrs(ctx.get_arena());

  while (ctx.matches(Class::Type::Attribute)) {
    attrs.insert(Attribute::parse(ctx));
  }

  result.attributes = attrs.get_view();

  if (ctx.matches(Class::Type::External)) {
    result.external = True;
    ctx.advance();
  }

  return result;
}
