// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/view/vector.hpp"
#include "perimortem/core/static/vector.hpp"
#include "perimortem/core/null_terminated.hpp"

namespace Ttx::Lexical {

// Class labels the valid value of a token in the smallest possible ctx.
// Tokens of different classes can't have any overlap and any arbitrary string
// of bytes is guaranteed to have a single valid tokenization in a single pass.
class Class {
 public:
  enum class Type : Bits_8 {
    // Sigils
    Public,          // public
    ConstPublic,     // @public
    Dynamic,         // expose
    ConstDynamic,    // @expose
    Hidden,          // hidden
    ConstHidden,     // @hidden
    Temporary,       // stack
    ConstTemporary,  // @stack
    Import,          // import
    Dialect,         // dialect

    // Compiler objects
    Comment,      // //
    Disabled,     // />
    Attribute,    // @
    CompileIf,    // @if
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
    PackingStart,  // (
    PackingEnd,    // )
    IndexStart,    // [ type args or layout
    IndexEnd,      // ]
    Assign,        // =
    AddAssign,     // +=
    SubAssign,     // -=
    Define,        // :
    TypeAccessOp,  // ::
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
    SliceOp,      // :[
    PackingOp,    // ,
    NotOp,        // !
    RangeOp,      // ...
    // The typical bit operators are reserved for future use.
    //
    // Bit ops are currently supported as method calls and always shrinks or
    // grows to the output type:
    // - `Bits_32 -> from(value) -> bit_and(Bits_8 -> from(0xFF))`
    // - `Bits_8 -> from(value) -> bit_or(Bits_32 -> from(0xFF000000))`
    AndOp,  // & reserved
    OrOp,   // | reserved

    // Fixed keywords
    And,  // `and` logical and (language level short-circuit)
    Or,   // `or` logical or (language level short-circuit)
    If,
    In,
    For,
    Break,
    Continue,
    New,
    Case,
    Else,
    Enum,
    External,
    Func,
    Init,
    Self,
    True,
    Alias,
    False,
    Match,
    While,
    Object,
    Return,
    Struct,
    Foreign,

    // Script types
    PackedData,
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

  constexpr auto is_one_of(Perimortem::Core::View::Vector<Type> values) const
      -> Bool {
    for (Count i = 0; i < values.get_size(); i++) {
      if (get_type() == values[i]) {
        return True;
      }
    }

    return False;
  }

  static constexpr auto is_sigil(Type value) -> Bool {
    switch (value) {
    case Type::Public:
    case Type::ConstPublic:
    case Type::Dynamic:
    case Type::ConstDynamic:
    case Type::Hidden:
    case Type::ConstHidden:
    case Type::Temporary:
    case Type::ConstTemporary:
      return True;
    default:
      return False;
    }
  }

  static constexpr auto is_type_ref(Type value) -> Bool {
    switch (value) {
    case Type::Type:
      return True;
    default:
      return False;
    }
  }

  constexpr auto is_sigil() const -> Bool { return is_sigil(type); }
  constexpr auto is_type_ref() const -> Bool { return is_type_ref(type); }

  constexpr auto get_type() const -> Type { return type; }

  static constexpr auto get_source_text(Type value)
      -> Perimortem::Core::View::Bytes {
    switch (value) {
    // Sigils
    case Type::Public:
      return "public"_view;
    case Type::ConstPublic:
      return "@public"_view;
    case Type::Dynamic:
      return "expose"_view;
    case Type::ConstDynamic:
      return "@expose"_view;
    case Type::Hidden:
      return "hidden"_view;
    case Type::ConstHidden:
      return "@hidden"_view;
    case Type::Temporary:
      return "stack"_view;
    case Type::ConstTemporary:
      return "@stack"_view;

    // Definition keywords.
    case Type::Object:
      return "object"_view;
    case Type::Struct:
      return "struct"_view;
    case Type::Enum:
      return "enum"_view;
    case Type::Foreign:
      return "foreign"_view;
    case Type::Alias:
      return "alias"_view;

    // Statement and import keywords
    case Type::If:
      return "if"_view;
    case Type::CompileIf:
      return "@if"_view;
    case Type::In:
      return "in"_view;
    case Type::For:
      return "for"_view;
    case Type::Break:
      return "break"_view;
    case Type::Continue:
      return "continue"_view;
    case Type::Match:
      return "match"_view;
    case Type::Case:
      return "case"_view;
    case Type::Else:
      return "else"_view;
    case Type::While:
      return "while"_view;
    case Type::Return:
      return "return"_view;
    case Type::Import:
      return "import"_view;
    case Type::Dialect:
      return "dialect"_view;
    case Type::Func:
      return "func"_view;
    case Type::External:
      return "external"_view;

    // Binary operators
    case Type::AddOp:
      return "+"_view;
    case Type::SubOp:
      return "-"_view;
    case Type::MulOp:
      return "*"_view;
    case Type::DivOp:
      return "/"_view;
    case Type::ModOp:
      return "%"_view;
    case Type::CmpOp:
      return "=="_view;
    case Type::NotEqOp:
      return "!="_view;
    case Type::LessOp:
      return "<"_view;
    case Type::GreaterOp:
      return ">"_view;
    case Type::LessEqOp:
      return "<="_view;
    case Type::GreaterEqOp:
      return ">="_view;
    case Type::And:
      return "and"_view;
    case Type::Or:
      return "or"_view;
    case Type::AndOp:
      return "&"_view;
    case Type::OrOp:
      return "|"_view;

    // Assignment operators
    case Type::Assign:
      return "="_view;
    case Type::AddAssign:
      return "+="_view;
    case Type::SubAssign:
      return "-="_view;

    // Address, packing, and parser operators
    case Type::ScopeStart:
      return "{"_view;
    case Type::ScopeEnd:
      return "}"_view;
    case Type::PackingStart:
      return "("_view;
    case Type::PackingEnd:
      return ")"_view;
    case Type::IndexStart:
      return "["_view;
    case Type::IndexEnd:
      return "]"_view;
    case Type::Define:
      return ":"_view;
    case Type::TypeAccessOp:
      return "::"_view;
    case Type::EndStatement:
      return ";"_view;
    case Type::CallOp:
      return "->"_view;
    case Type::AddressOp:
      return "."_view;
    case Type::SwizzleOp:
      return ".["_view;
    case Type::SliceOp:
      return ":["_view;
    case Type::PackingOp:
      return ","_view;
    case Type::NotOp:
      return "!"_view;
    case Type::RangeOp:
      return "..."_view;
    case Type::Discard:
      return "_"_view;

    // Other fixed keywords and markers
    case Type::New:
      return "new"_view;
    case Type::Init:
      return "init"_view;
    case Type::Self:
      return "self"_view;
    case Type::True:
      return "true"_view;
    case Type::False:
      return "false"_view;

    // Literal markers
    case Type::Bytes:
      return "0x["_view;
    case Type::Embedded:
      return "$["_view;

    // Other fixed markers
    case Type::Attribute:
      return "@"_view;
    case Type::Comment:
      return "//"_view;
    case Type::Disabled:
      return "/>"_view;

    // Return empty for anything that should be sourced from the token itself.
    default:
      return ""_view;
    }
  }

  // Gets a human readable name for the class.
  auto get_name() const -> Perimortem::Core::View::Bytes;

  // Gets the source-level text for tokens whose type encodes a fixed spelling
  // such as an operator, keyword, or builtin label.
  // Returns an empty view for token types that carry their text in the token
  // itself (identifiers and literals).
  constexpr auto get_source_text() const -> Perimortem::Core::View::Bytes {
    return get_source_text(type);
  }

 private:
  Type type = Type::EndOfStream;
};

}  // namespace Ttx::Lexical
