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
    Discard,      // Discard (_)

    // Data objects
    Numeric,   // 0-1 no decimal
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
    TypedAssign,   // :=  inferred-type assignment (like auto)
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
    NotEqOp,      // !=
    CallOp,       // ->
    AddressOp,    // .
    SwizzleOp,    // .[
    SliceOp,      // +:
    PackingOp,    // ,
    NotOp,        // !
    RangeOp,      // ...
    GeneratorOp,  // =>
    // The typical bit operators are reserved for future use.
    //
    // Bit ops are currently supported as method calls and always shrinks or
    // grows to the output type:
    // - `Bits_32(value) -> bit_and(Bits_8(0xFF))`
    // - `Bits_8(value) -> bit_or(Bits_32(0xFF000000))`
    AndOp,  // & reserved
    OrOp,   // | reserved

    // Keywords
    As,
    And,  // `and` logical and (language level short-circuit)
    Or,   // `or` logical or (language level short-circuit)
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
    Unknown,
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

  static constexpr auto is_sigil(Type value) -> Bool {
    switch (value) {
    case Type::Constant:
    case Type::Detail:
    case Type::Public:
    case Type::Dynamic:
    case Type::Hidden:
    case Type::Temporary:
      return True;
    default:
      return False;
    }
  }

  constexpr auto is_sigil() const -> Bool { return is_sigil(type); }

  constexpr auto get_type() const -> Type { return type; }

  // Gets a human readable name for the class.
  auto get_name() const -> Perimortem::Core::View::Bytes;

  // Gets the source-level text for tokens whose type encodes the operator or
  // keyword (e.g. AddOp → "+", Constant → "frozen", Library → "Library").
  // Returns an empty view for token types that carry their text in the token
  // itself (identifiers, literals, comments).
  auto get_source_text() const -> Perimortem::Core::View::Bytes;

 private:
  Type type = Type::EndOfStream;
};

}  // namespace Tetrodotoxin::Lexical
