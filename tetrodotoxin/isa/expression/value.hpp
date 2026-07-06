// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"

#include "tetrodotoxin/isa/context.hpp"
#include "tetrodotoxin/isa/expression.hpp"

namespace Tetrodotoxin::Isa {

class Expression::Value {
 public:
  enum class Kind : Bits_8 {
    Empty,
    String,
    Reference,
  };

  constexpr Value() = default;

  static auto evaluate(Ttx::Lexical::Cursor& cursor, Context& context)
      -> Value;
  static constexpr auto string(Perimortem::Core::View::Bytes value) -> Value {
    return Value(Kind::String, value);
  }
  static constexpr auto reference(Perimortem::Core::View::Bytes value)
      -> Value {
    return Value(Kind::Reference, value);
  }

  constexpr auto get_kind() const -> Kind { return kind; }
  constexpr auto get_value() const -> Perimortem::Core::View::Bytes {
    return value;
  }
  constexpr auto is_empty() const -> Bool { return kind == Kind::Empty; }

 private:
  constexpr Value(Kind kind, Perimortem::Core::View::Bytes value)
      : kind(kind), value(value) {}

  Kind kind = Kind::Empty;
  Perimortem::Core::View::Bytes value;
};

}  // namespace Tetrodotoxin::Isa
