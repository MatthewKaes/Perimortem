// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "tetrodotoxin/syntax/expression/expression.hpp"

namespace Tetrodotoxin::Syntax::Expression {

class Binary : public Expression {
 public:
  Binary(Lexical::Class operator_class, Expression* left, Expression* right)
      : Expression(operator_class), left(left), right(right) {}

  static auto parse(Context& ctx) -> Expression*;
  static auto precedence(Lexical::Class klass) -> Count;

  auto get_kind() const -> Kind override { return Kind::Binary; }
  auto get_left() const -> const Expression* override { return left; }
  auto get_right() const -> const Expression* override { return right; }

 private:
  Expression* left = nullptr;
  Expression* right = nullptr;
};

}  // namespace Tetrodotoxin::Syntax::Expression
