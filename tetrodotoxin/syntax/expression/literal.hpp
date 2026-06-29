// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "tetrodotoxin/syntax/expression/expression.hpp"

namespace Tetrodotoxin::Syntax::Expression {

class Literal : public Expression {
 public:
  Literal(Lexical::Class klass, Perimortem::Core::View::Bytes text)
      : Expression(klass, text) {}

  static auto inner_text(Perimortem::Core::View::Bytes text)
      -> Perimortem::Core::View::Bytes;

  auto get_kind() const -> Kind override { return Kind::Literal; }
};

}  // namespace Tetrodotoxin::Syntax::Expression
