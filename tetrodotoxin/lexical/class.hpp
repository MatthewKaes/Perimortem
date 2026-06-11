// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/view/vector.hpp"
#include "perimortem/core/static/vector.hpp"

namespace Tetrodotoxin::Lexical {

// Class labels the valid value of a token in the smallest possible context.
// Tokens of different classes can't have any overlap and any arbitrary string
// of bytes is guaranteed to have a single valid tokenization in a single pass.
class Class {
 public:
  enum class Type : Bits_8 {
    // Sigils
    Constant,   // frozen
    Detail,     // detail
    Public,     // public
    Dynamic,    // expose
    Hidden,     // hidden
    Temporary,  // stack
    Import,     // import

    // Compiler objects
    Comment,      // //
    Disabled,     // />
    Attribute,    // @
    Addressable,  // Any symbol that starts with [a-z]
    Type,         // Any symbol that starts with [A-Z]
    PackedName,   // Any identifier for a pack
    Discard,      // Discard (_)

    // Data objects
    Numeric,   // 0-1 no decemial
    Float,     // 9. 0.9 as floats, 9.. Float + Access Op, 9...
    String,    // " " (does not embed null terminator)
    Bytes,     // 0x[FF FF FF]
    Embedded,  // $[path/to/file]

    // Parser objects
    ScopeStart,    // {
    ScopeEnd,      // }
    GroupStart,    // (
    GroupEnd,      // )
    IndexStart,    // [
    IndexEnd,      // ]
    Assign,        // =
    AddAssign,     // +=
    SubAssign,     // -=
    Define,        // :
    EndStatement,  // ;

    // Operators
    AddOp,        // +
    SubOp,        // -
    DivOp,        // /
    MulOp,        // *
    ModOp,        // %
    LessOp,       // <
    GreaterOp,    // >
    LessEqOp,     // <=
    GreaterEqOp,  // >=
    CmpOp,        // ==
    CallOp,       // ->
    AddressOp,    // .
    SwizzleOp,    // .[
    PackingOp,    // ,
    NotOp,        // !
    AndOp,        // &
    OrOp,         // |
    RangeOp,      // ...
    GeneratorOp,  // =>

    // Keywords
    As,
    If,
    In,
    For,
    New,
    Case,
    Else,
    Enum,
    Func,
    Init,
    Self,
    True,
    Alias,
    False,
    Match,
    Using,
    While,
    Shader,
    Entity,
    Object,
    Return,
    Struct,
    Library,
    Package,

    // Script types
    EndOfStream,
  };

  constexpr Class() = default;
  constexpr Class(Type type) : type(type) {}

  constexpr auto operator==(const Class& rhs) const -> Bool {
    return get_type() == rhs.get_type();
  }

  constexpr auto operator==(Type type) const -> Bool {
    return get_type() == type;
  }

  constexpr auto operator!=(const Class& rhs) const -> Bool {
    return get_type() != rhs.get_type();
  }

  constexpr auto operator!=(Type type) const -> Bool {
    return get_type() != type;
  }

  constexpr auto is_one_of(Perimortem::Core::View::Vector<Class> values) const
      -> Bool {
    for (Count i = 0; i < values.get_size(); i++) {
      if (get_type() == values[i].get_type()) {
        return True;
      }
    }

    return False;
  }

  template <Count size>
  constexpr auto is_one_of(
      Perimortem::Core::Static::Vector<Class, size> values) const -> Bool {
    for (Count i = 0; i < values.get_size(); i++) {
      if (get_type() == values[i].get_type()) {
        return True;
      }
    }

    return False;
  }

  constexpr auto get_type() const -> Type { return type; }

  // Gets a human redable name for the class
  auto get_name() const -> Perimortem::Core::View::Bytes;

 private:
  Type type = Type::EndOfStream;
};

}  // namespace Tetrodotoxin::Lexical
