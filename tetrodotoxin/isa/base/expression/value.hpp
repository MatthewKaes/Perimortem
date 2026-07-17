// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/view/vector.hpp"
#include "perimortem/core/static/union.hpp"

#include "ttx/lexical/cursor.hpp"
#include "ttx/lexical/token.hpp"
#include "ttx/type.hpp"

namespace Tetrodotoxin::Isa::Base {

class Context;

}  // namespace Tetrodotoxin::Isa::Base

namespace Tetrodotoxin::Isa::Base::Expression {

class Pack;

class Value {
 public:
  // TODO: Operators should either be an extensible class set or it's own closed
  // class form that handles operations rather than stuffing it in value.
  enum class Operator : Unsigned_8 {
    None,
    Add,
    Subtract,
    Multiply,
    Divide,
    Remainder,
    Equal,
  };

  enum class Kind : Unsigned_8 {
    Empty,
    String,
    Bytes,
    Boolean,
    Reference,
    Numeric,
    Real,
    Pack,
    Call,
    Binary,
    Type,
  };

 private:
  class PackPayload {
   public:
    explicit PackPayload(const Pack& value) : value(value) {}
    constexpr auto get_value() const -> const Pack& { return value; }

   private:
    const Pack& value;
  };

  class CallPayload {
   public:
    CallPayload(
        const Value& owner,
        Perimortem::Core::View::Bytes name,
        const Pack& arguments)
        : owner(owner), name(name), arguments(arguments) {}

    constexpr auto get_owner() const -> const Value& { return owner; }
    constexpr auto get_name() const -> Perimortem::Core::View::Bytes {
      return name;
    }
    constexpr auto get_arguments() const -> const Pack& { return arguments; }

   private:
    const Value& owner;
    Perimortem::Core::View::Bytes name;
    const Pack& arguments;
  };

  class BinaryPayload {
   public:
    BinaryPayload(Operator op, const Value& left, const Value& right)
        : op(op), left(left), right(right) {}

    constexpr auto get_operator() const -> Operator { return op; }
    constexpr auto get_left() const -> const Value& { return left; }
    constexpr auto get_right() const -> const Value& { return right; }

   private:
    Operator op;
    const Value& left;
    const Value& right;
  };

  class TypePayload {
   public:
    explicit TypePayload(const Ttx::Type& value) : value(value) {}
    constexpr auto get_value() const -> const Ttx::Type& { return value; }

   private:
    const Ttx::Type& value;
  };

 public:
  constexpr Value() = default;

  static auto evaluate(Ttx::Lexical::Cursor& cursor, Context& context) -> Value;
  static constexpr auto string(Perimortem::Core::View::Bytes value) -> Value {
    return Value(Kind::String, value);
  }

  static constexpr auto boolean(Bool value) -> Value {
    return Value(Kind::Boolean, value);
  }

  static constexpr auto reference(
      Perimortem::Core::View::Bytes value,
      Perimortem::Core::View::Vector<Ttx::Lexical::Token> tokens) -> Value {
    return Value(Kind::Reference, value, tokens);
  }

  static constexpr auto bytes(Perimortem::Core::View::Bytes value) -> Value {
    return Value(Kind::Bytes, value);
  }

  // A Type value borrows the canonical identity owned by the active resolution
  // transaction. This is the same lifetime as other expression references and
  // is sufficient for semantic inspection, completion, and lowering. A
  // terminal must choose a durable target representation before materializing
  // the value at runtime. Compiler addresses are not a runtime ABI.
  static auto type(const Ttx::Type& value) -> Value { return Value(value); }

  static constexpr auto numeric(
      Perimortem::Core::View::Bytes value,
      Bool negative = False) -> Value {
    Value result(Kind::Numeric, value);
    result.negative = negative;
    return result;
  }

  static constexpr auto floating(
      Perimortem::Core::View::Bytes value,
      Bool negative = False) -> Value {
    Value result(Kind::Real, value);
    result.negative = negative;
    return result;
  }

  static auto pack(const Pack& value) -> Value {
    return Value(PackPayload(value));
  }

  static auto call(
      const Value& owner,
      Perimortem::Core::View::Bytes name,
      const Pack& arguments) -> Value {
    return Value(CallPayload(owner, name, arguments));
  }

  static auto binary(Operator op, const Value& left, const Value& right)
      -> Value {
    return Value(BinaryPayload(op, left, right));
  }

  constexpr auto get_kind() const -> Kind { return kind; }
  constexpr auto get_operator() const -> Operator {
    return payload.find<BinaryPayload>()->get_operator();
  }
  constexpr auto get_value() const -> Perimortem::Core::View::Bytes {
    return value;
  }

  constexpr auto get_flag() const -> Bool { return flag; }
  constexpr auto is_negative() const -> Bool { return negative; }
  constexpr auto get_tokens() const
      -> Perimortem::Core::View::Vector<Ttx::Lexical::Token> {
    return tokens;
  }

  constexpr auto get_pack() const -> const Pack& {
    return payload.find<PackPayload>()->get_value();
  }
  constexpr auto get_call_owner() const -> const Value& {
    return payload.find<CallPayload>()->get_owner();
  }
  constexpr auto get_call_name() const -> Perimortem::Core::View::Bytes {
    return payload.find<CallPayload>()->get_name();
  }

  constexpr auto get_call_arguments() const -> const Pack& {
    return payload.find<CallPayload>()->get_arguments();
  }

  constexpr auto get_left() const -> const Value& {
    return payload.find<BinaryPayload>()->get_left();
  }
  constexpr auto get_right() const -> const Value& {
    return payload.find<BinaryPayload>()->get_right();
  }
  constexpr auto get_type() const -> const Ttx::Type& {
    return payload.find<TypePayload>()->get_value();
  }
  constexpr auto is_empty() const -> Bool { return kind == Kind::Empty; }

 private:
  constexpr Value(Kind kind) : kind(kind) {}
  constexpr Value(Kind kind, Perimortem::Core::View::Bytes value)
      : kind(kind), value(value) {}
  constexpr Value(Kind kind, Bool value) : kind(kind), flag(value) {}
  constexpr Value(
      Kind kind,
      Perimortem::Core::View::Bytes value,
      Perimortem::Core::View::Vector<Ttx::Lexical::Token> tokens)
      : kind(kind), value(value), tokens(tokens) {}
  explicit Value(PackPayload value) : kind(Kind::Pack), payload(value) {}
  explicit Value(CallPayload value) : kind(Kind::Call), payload(value) {}
  explicit Value(BinaryPayload value) : kind(Kind::Binary), payload(value) {}
  explicit Value(const Ttx::Type& value)
      : kind(Kind::Type), payload(TypePayload(value)) {}

  Kind kind = Kind::Empty;
  Perimortem::Core::View::Bytes value;
  Bool flag = False;
  Bool negative = False;
  Perimortem::Core::View::Vector<Ttx::Lexical::Token> tokens;
  Perimortem::Core::Static::
      Union<PackPayload, CallPayload, BinaryPayload, TypePayload>
          payload;
};

}  // namespace Tetrodotoxin::Isa::Base::Expression
