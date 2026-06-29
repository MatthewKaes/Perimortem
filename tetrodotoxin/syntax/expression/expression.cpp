// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/syntax/expression/expression.hpp"

#include "tetrodotoxin/syntax/context.hpp"
#include "tetrodotoxin/syntax/expression/binary.hpp"

using namespace Perimortem::Core;
using namespace Tetrodotoxin::Syntax;
using namespace Tetrodotoxin::Lexical;

auto Expression::Expression::make(Context& ctx, Class klass) -> Expression* {
  return Expression::construct<Expression>(ctx, klass);
}

auto Expression::Expression::parse(Context& ctx) -> Expression* {
  return Binary::parse(ctx);
}

auto Expression::Expression::get_type_ref() const -> const Ref& {
  static Ref empty;
  return empty;
}
