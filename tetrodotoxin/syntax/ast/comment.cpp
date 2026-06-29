// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/syntax/ast/comment.hpp"

#include "perimortem/memory/managed/vector.hpp"

#include "tetrodotoxin/syntax/context.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Tetrodotoxin::Syntax;
using namespace Tetrodotoxin::Lexical;

auto Ast::Comment::parse(Context& ctx) -> Comment {
  Managed::Vector<View::Bytes> lines(ctx.get_arena());

  while (ctx.matches(Class::Type::Comment)) {
    lines.insert(ctx.get_current().get_text());
    ctx.advance();
  }

  Comment result;
  result.lines = lines.get_view();
  return result;
}
