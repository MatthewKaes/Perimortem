// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "tetrodotoxin/syntax/expression/expression.hpp"

namespace Tetrodotoxin::Syntax::Expression {

class Addressable : public Expression {
 public:
  Addressable(Lexical::Class klass, Perimortem::Core::View::Bytes text)
      : Expression(klass, text) {}

  auto get_kind() const -> Kind override { return Kind::Addressable; }
};

}  // namespace Tetrodotoxin::Syntax::Expression
