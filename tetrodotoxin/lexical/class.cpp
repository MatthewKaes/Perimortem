// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/lexical/class.hpp"

#include "perimortem/core/null_terminated.hpp"

using namespace Tetrodotoxin::Lexical;

auto Class::get_source_text() const -> Perimortem::Core::View::Bytes {
  switch (type) {
  // Sigil keywords
  case Type::Constant:
    return "frozen"_view;
  case Type::Detail:
    return "detail"_view;
  case Type::Public:
    return "public"_view;
  case Type::Dynamic:
    return "expose"_view;
  case Type::Hidden:
    return "hidden"_view;
  case Type::Temporary:
    return "stack"_view;

  // Package kind keywords
  case Type::Library:
    return "Library"_view;
  case Type::Shader:
    return "Shader"_view;
  case Type::Entity:
    return "Entity"_view;
  case Type::Object:
    return "Object"_view;
  case Type::Struct:
    return "Struct"_view;

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

  // Return empty for anything that should be sourced from the token itself.
  default:
    return ""_view;
  }
}

auto Class::get_name() const -> Perimortem::Core::View::Bytes {
  switch (type) {
  case Type::Comment:
    return "comment"_view;
  case Type::Disabled:
    return "disabled"_view;
  case Type::Unknown:
    return "unknown token"_view;
  case Type::String:
    return "string"_view;
  case Type::Bytes:
    return "hex byte array literal"_view;
  case Type::Embedded:
    return "embedded file literal"_view;
  case Type::Numeric:
    return "64bit integer constant"_view;
  case Type::Float:
    return "64bit IEEE float constant"_view;
  case Type::Attribute:
    return "compiler directive"_view;
  case Type::Addressable:
    return "addressable identifier"_view;
  case Type::Type:
    return "type"_view;
  case Type::ScopeStart:
    return "{"_view;
  case Type::ScopeEnd:
    return "}"_view;
  case Type::GroupStart:
    return "("_view;
  case Type::GroupEnd:
    return ")"_view;
  case Type::IndexStart:
    return "["_view;
  case Type::IndexEnd:
    return "]"_view;
  case Type::Assign:
    return "assignment"_view;
  case Type::AddAssign:
    return "AddAssign"_view;
  case Type::SubAssign:
    return "mutate addressable (-=)"_view;
  case Type::Define:
    return "definition (:)"_view;
  case Type::TypedAssign:
    return "type-inferred assignment (:=)"_view;
  case Type::EndStatement:
    return "end statement"_view;
  case Type::AddOp:
    return "addition operator (+)"_view;
  case Type::SubOp:
    return "subtraction operator (-)"_view;
  case Type::DivOp:
    return "division operator (/)"_view;
  case Type::MulOp:
    return "multiplication operator (*)"_view;
  case Type::ModOp:
    return "modulo operator (%)"_view;
  case Type::LessOp:
    return "comparison operator (<)"_view;
  case Type::GreaterOp:
    return "comparison operator (>)"_view;
  case Type::LessEqOp:
    return "comparison operator (<=)"_view;
  case Type::GreaterEqOp:
    return "comparison operator (>=)"_view;
  case Type::CmpOp:
    return "comparison operator (==)"_view;
  case Type::NotEqOp:
    return "comparison operator (!=)"_view;
  case Type::CallOp:
    return "Call operator (->)"_view;
  case Type::RangeOp:
    return "range operator (...)"_view;
  case Type::GeneratorOp:
    return "static generator (=>)"_view;
  case Type::PackingOp:
    return "packing operator (,)"_view;
  case Type::AddressOp:
    return "addressable operator (.)"_view;
  case Type::SwizzleOp:
    return "swizzle operator (.[)"_view;
  case Type::SliceOp:
    return "slice operator (+:)"_view;
  case Type::AndOp:
    return "bitwise-and (&) — reserved, use -> bit_and()"_view;
  case Type::OrOp:
    return "bitwise-or (|) — reserved, use -> bit_or()"_view;
  case Type::NotOp:
    return "negation"_view;
  case Type::And:
    return "logical and"_view;
  case Type::Or:
    return "logical or"_view;
  case Type::If:
    return "if"_view;
  case Type::In:
    return "in"_view;
  case Type::For:
    return "for"_view;
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
  case Type::Func:
    return "function definition"_view;
  case Type::Object:
    return "managed class definition"_view;
  case Type::Struct:
    return "storage class definition"_view;
  case Type::Enum:
    return "enum definition"_view;
  case Type::Library:
    return "ttx library declaration"_view;
  case Type::Shader:
    return "ttx shader definition"_view;
  case Type::Entity:
    return "ttx entity declaration"_view;
  case Type::Using:
    return "import package statement"_view;
  case Type::True:
    return "boolean value `true`"_view;
  case Type::False:
    return "boolean value `false`"_view;
  case Type::Package:
    return "package definition"_view;
  case Type::Init:
    return "stack creator"_view;
  case Type::New:
    return "heap creator"_view;
  case Type::Self:
    return "`self` accessor"_view;
  case Type::As:
    return "external alias declaration"_view;
  case Type::Alias:
    return "internal alias declaration"_view;
  case Type::EndOfStream:
    return "end of file"_view;
  case Type::Constant:
    return "frozen"_view;
  case Type::Detail:
    return "detail"_view;
  case Type::Public:
    return "public"_view;
  case Type::Dynamic:
    return "expose"_view;
  case Type::Hidden:
    return "hidden"_view;
  case Type::Temporary:
    return "stack"_view;
  case Type::Import:
    return "import"_view;
  default:
    return "unknown"_view;
  }
}
