// Perimortem Engine
// Copyright © Matt Kaes

#include "ttx/lexical/class.hpp"

#include "perimortem/core/null_terminated.hpp"

using namespace Ttx::Lexical;

auto Class::get_name() const -> Perimortem::Core::View::Bytes {
  switch (type) {
  // Sigils
  case Type::Public:
    return "public addressable qualifier"_view;
  case Type::ConstPublic:
    return "public addressable constant qualifier"_view;
  case Type::Dynamic:
    return "exposed addressable qualifier"_view;
  case Type::ConstDynamic:
    return "exposed addressable constant qualifier"_view;
  case Type::Hidden:
    return "hidden addressable qualifier"_view;
  case Type::ConstHidden:
    return "hidden addressable constant qualifier"_view;
  case Type::Temporary:
    return "stack addressable qualifier"_view;
  case Type::ConstTemporary:
    return "stack addressable constant qualifier"_view;

  // Definition keywords
  case Type::Object:
    return "`object` definition keyword"_view;
  case Type::Struct:
    return "`struct` definition keyword"_view;
  case Type::Enum:
    return "`enum` definition keyword"_view;
  case Type::Foreign:
    return "`foreign` ABI definition keyword"_view;
  case Type::Alias:
    return "`alias` definition keyword"_view;

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
    return "dialect declaration"_view;
  case Type::Func:
    return "function definition"_view;
  case Type::External:
    return "external function declaration"_view;

  // Binary operators
  case Type::AddOp:
    return "addition operator (+)"_view;
  case Type::SubOp:
    return "subtraction operator (-)"_view;
  case Type::MulOp:
    return "multiplication operator (*)"_view;
  case Type::DivOp:
    return "division operator (/)"_view;
  case Type::ModOp:
    return "modulo operator (%)"_view;
  case Type::CmpOp:
    return "comparison operator (==)"_view;
  case Type::NotEqOp:
    return "comparison operator (!=)"_view;
  case Type::LessOp:
    return "comparison operator (<)"_view;
  case Type::GreaterOp:
    return "comparison operator (>)"_view;
  case Type::LessEqOp:
    return "comparison operator (<=)"_view;
  case Type::GreaterEqOp:
    return "comparison operator (>=)"_view;
  case Type::And:
    return "logical and"_view;
  case Type::Or:
    return "logical or"_view;
  case Type::AndOp:
    return "bitwise-and (&) — reserved, use -> bit_and()"_view;
  case Type::OrOp:
    return "bitwise-or (|) — reserved, use -> bit_or()"_view;

  // Assignment operators
  case Type::Assign:
    return "mutate addressable (=)"_view;
  case Type::AddAssign:
    return "mutate addressable (+=)"_view;
  case Type::SubAssign:
    return "mutate addressable (-=)"_view;

  // Address, packing, and parser operators
  case Type::ScopeStart:
    return "scope operator `{`"_view;
  case Type::ScopeEnd:
    return "closing operator `}`"_view;
  case Type::PackingStart:
    return "packing operator `(`"_view;
  case Type::PackingEnd:
    return "closing operator `)`"_view;
  case Type::IndexStart:
    return "type argument or layout operator `[`"_view;
  case Type::IndexEnd:
    return "closing type argument or layout operator `]`"_view;
  case Type::Define:
    return "definition `:`"_view;
  case Type::TypeAccessOp:
    return "type access operator `::`"_view;
  case Type::EndStatement:
    return "end statement"_view;
  case Type::CallOp:
    return "Call operator (->)"_view;
  case Type::RangeOp:
    return "range operator (...)"_view;
  case Type::PackingOp:
    return "packing operator (,)"_view;
  case Type::AddressOp:
    return "addressable operator `.`"_view;
  case Type::SwizzleOp:
    return "swizzle operator `.[`"_view;
  case Type::SliceOp:
    return "index or slice operator `:[`"_view;
  case Type::NotOp:
    return "negation"_view;
  case Type::Discard:
    return "discard"_view;

  // Other fixed keywords and markers
  case Type::New:
    return "heap creator"_view;
  case Type::Init:
    return "stack creator"_view;
  case Type::Self:
    return "`self` accessor"_view;
  case Type::True:
    return "boolean value `true`"_view;
  case Type::False:
    return "boolean value `false`"_view;

  // Literal markers
  case Type::Bytes:
    return "hex byte array literal"_view;
  case Type::Embedded:
    return "embedded file literal"_view;

  // Other fixed markers
  case Type::Attribute:
    return "compiler directive"_view;
  case Type::Comment:
    return "comment"_view;
  case Type::Disabled:
    return "disabled"_view;

  // Other token classes
  case Type::String:
    return "string"_view;
  case Type::Numeric:
    return "64bit integer constant"_view;
  case Type::Float:
    return "64bit IEEE float constant"_view;
  case Type::Addressable:
    return "addressable identifier"_view;
  case Type::Type:
    return "type"_view;
  case Type::PackedData:
    return "packed data source"_view;
  case Type::EndOfStream:
    return "end of file"_view;

    // Surface tokenization errors to the user which should only happen on
    // incorrectly formatted UTF files, but there could be edge cases.
  case Type::Unknown:
  default:
    return "unknown token"_view;
  }
}
