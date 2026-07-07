// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/view/vector.hpp"

#include "tetrodotoxin/isa/context.hpp"
#include "tetrodotoxin/isa/expression.hpp"
#include "ttx/lexical/token.hpp"

namespace Tetrodotoxin::Isa {

class Expression::Value {
 public:
  // TODO: Operators should either be an extensible class set or it's own closed
  // class form that handles operations rather than stuffing it in value.
  enum class Operator : Bits_8 {
    None,
    Add,
    Subtract,
    Multiply,
    Divide,
  };

  enum class Kind : Bits_8 {
    Empty,
    String,
    Reference,
    Numeric,
    Real,
    Pack,
    Binary,
  };

  constexpr Value() = default;

  static auto evaluate(Ttx::Lexical::Cursor& cursor, Context& context) -> Value;
  static constexpr auto string(Perimortem::Core::View::Bytes value) -> Value {
    return Value(Kind::String, value);
  }
  static constexpr auto reference(
      Perimortem::Core::View::Bytes value,
      Perimortem::Core::View::Vector<Ttx::Lexical::Token> tokens) -> Value {
    return Value(Kind::Reference, value, tokens);
  }
  static constexpr auto numeric(Perimortem::Core::View::Bytes value) -> Value {
    return Value(Kind::Numeric, value);
  }
  static constexpr auto floating(Perimortem::Core::View::Bytes value) -> Value {
    return Value(Kind::Real, value);
  }
  static constexpr auto pack(const Pack& value) -> Value {
    Value result(Kind::Pack);
    result.pack_value = &value;
    return result;
  }
  static constexpr auto
      binary(Operator op, const Value& left, const Value& right) -> Value {
    Value result(Kind::Binary);
    result.op = op;
    result.left = &left;
    result.right = &right;
    return result;
  }

  constexpr auto get_kind() const -> Kind { return kind; }
  constexpr auto get_operator() const -> Operator { return op; }
  constexpr auto get_value() const -> Perimortem::Core::View::Bytes {
    return value;
  }
  constexpr auto get_tokens() const
      -> Perimortem::Core::View::Vector<Ttx::Lexical::Token> {
    return tokens;
  }
  constexpr auto get_pack() const -> const Pack* { return pack_value; }
  constexpr auto get_left() const -> const Value* { return left; }
  constexpr auto get_right() const -> const Value* { return right; }
  constexpr auto is_empty() const -> Bool { return kind == Kind::Empty; }

 private:
  constexpr Value(Kind kind) : kind(kind) {}
  constexpr Value(Kind kind, Perimortem::Core::View::Bytes value)
      : kind(kind), value(value) {}
  constexpr Value(
      Kind kind,
      Perimortem::Core::View::Bytes value,
      Perimortem::Core::View::Vector<Ttx::Lexical::Token> tokens)
      : kind(kind), value(value), tokens(tokens) {}

  Kind kind = Kind::Empty;
  Operator op = Operator::None;
  Perimortem::Core::View::Bytes value;
  Perimortem::Core::View::Vector<Ttx::Lexical::Token> tokens;
  const Pack* pack_value = nullptr;
  const Value* left = nullptr;
  const Value* right = nullptr;
};

}  // namespace Tetrodotoxin::Isa
