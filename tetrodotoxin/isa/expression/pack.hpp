// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/vector.hpp"

#include "tetrodotoxin/isa/expression/value.hpp"

namespace Tetrodotoxin::Isa {

class Expression::Pack {
 public:
  constexpr Pack() = default;
  explicit constexpr Pack(Perimortem::Core::View::Vector<Value> values)
      : values(values) {}

  static auto evaluate(Ttx::Lexical::Cursor& cursor, Context& context)
      -> const Pack*;

  constexpr auto get_values() const -> Perimortem::Core::View::Vector<Value> {
    return values;
  }

 private:
  Perimortem::Core::View::Vector<Value> values;
};

}  // namespace Tetrodotoxin::Isa
