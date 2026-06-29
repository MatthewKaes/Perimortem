// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/syntax/ast/value.hpp"

using namespace Perimortem::Core;
using namespace Tetrodotoxin::Syntax;

auto Ast::Value::parse(Context& ctx) -> Value {
  Value result;
  result.expr = Expression::Expression::parse(ctx);
  return result;
}

auto Ast::Value::from_expr(Expression::Expression* expr) -> Value {
  Value result;
  result.expr = expr;
  return result;
}
