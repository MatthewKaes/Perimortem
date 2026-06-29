// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "tetrodotoxin/syntax/expression/expression.hpp"

namespace Tetrodotoxin::Syntax::Expression {

class Data : public Expression {
 public:
  explicit Data(Perimortem::Core::View::Vector<Expression*> values)
      : Expression(Lexical::Class::Type::ScopeStart), values(values) {}

  auto get_kind() const -> Kind override { return Kind::Data; }
  auto get_args() const
      -> Perimortem::Core::View::Vector<Expression*> override {
    return values;
  }

 private:
  Perimortem::Core::View::Vector<Expression*> values;
};

}  // namespace Tetrodotoxin::Syntax::Expression
