// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "tetrodotoxin/syntax/expression/expression.hpp"

namespace Tetrodotoxin::Syntax::Expression {

class Unary : public Expression {
 public:
  Unary(Lexical::Class operator_class, Expression* right)
      : Expression(operator_class), right(right) {}

  auto get_kind() const -> Kind override { return Kind::Unary; }
  auto get_right() const -> const Expression* override { return right; }

 private:
  Expression* right = nullptr;
};

}  // namespace Tetrodotoxin::Syntax::Expression
