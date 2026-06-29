// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "tetrodotoxin/syntax/context.hpp"
#include "tetrodotoxin/syntax/expression/expression.hpp"

namespace Tetrodotoxin::Syntax::Ast {

class Value {
 public:
  static auto parse(Context& ctx) -> Value;
  static auto from_expr(Expression::Expression* expr) -> Value;

  auto get_expr() const -> const Expression::Expression* { return expr; }
  auto is_empty() const -> Bool { return expr == nullptr; }

 private:
  Expression::Expression* expr = nullptr;
};

}  // namespace Tetrodotoxin::Syntax::Ast
