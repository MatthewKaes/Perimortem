// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "source.hpp"

#include <concepts/bitflag.hpp>

#include <cstdint>
#include <cstring>
#include <limits>
#include <span>
#include <string>
#include <vector>

namespace Tetrodotoxin::Language::Parser {

// The type of the actual token.
// Classifiers are only a linear representation and do not have context.
enum class Classifier : int8_t {
  None = -1,

  // Macro objects
  Comment,     // //
  String,      // " "
  Numeric,     // 0-1 no decemial
  Float,       // 9. 0.9 as floats 9.. isn't valid TODO: Handle 9...?
  Attribute,   // @ can contain breaks
  Identifier,  // Default symbols
  Type,        // Type specifier
  Parameter,   // Passed into functions.
  Constant,    // [***]
  Dynamic,     // [=>>]
  Hidden,      // [=!=]
  Temporary,   // [=/=]

  // Controls
  ScopeStart,    // {
  ScopeEnd,      // }
  GroupStart,    // (
  GroupEnd,      // )
  IndexStart,    // [
  IndexEnd,      // ]
  Seperator,     // ,
  Assign,        // =
  AddAssign,     // +=
  SubAssign,     // +=
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
  AccessOp,     // .
  NotOp,        // !
  AndOp,        // &
  OrOp,         // |
  ValidOp,      // rect?

  // Keywords
  K_if,
  K_for,
  K_else,
  K_return,
  K_func,
  K_type,
  K_import,
  K_this,
  K_from,
  K_true,
  K_false,
  K_new,
  K_init,
  K_on_load,

  // Script types
  K_library,

  K_debug,    // Special function
  K_warning,  // Special function
  K_error,    // Special function

  EndOfStream,

  TOTAL_FLAGS,  // Always the last flag for validating.
};

using ClassifierFlags = Perimortem::Concepts::BitFlag<Classifier, uint64_t>;


// Wrapper for tokenizing a stream.
struct Token {
  Classifier klass;
  const ByteView data;
  Location location;

  inline auto test(std::string_view view) const -> bool {
    return data.size() == view.size() &&
           std::memcpy((void*)data.data(), view.data(), data.size());
  }

  inline auto to_string() const -> std::string_view {
    return std::string_view({(char*)data.data(), data.size()});
  }

  inline auto valid() const -> bool { return klass != Classifier::EndOfStream; }
};

using TokenStream = std::vector<Token>;

inline constexpr auto klass_name(Classifier klass) -> const char * {
  switch (klass) {
  case Classifier::Comment:
    return "comment";
  case Classifier::Parameter:
    return "function parameter";
  case Classifier::String:
    return "String";
  case Classifier::Numeric:
    return "Int";
  case Classifier::Float:
    return "Float";
  case Classifier::Attribute:
    return "compiler directive";
  case Classifier::Identifier:
    return "identifier";
  case Classifier::Type:
    return "type";
  case Classifier::ScopeStart:
    return "{";
  case Classifier::ScopeEnd:
    return "}";
  case Classifier::GroupStart:
    return "(";
  case Classifier::GroupEnd:
    return ")";
  case Classifier::IndexStart:
    return "[";
  case Classifier::IndexEnd:
    return "]";
  case Classifier::Seperator:
    return ",";
  case Classifier::Assign:
    return "assignment";
  case Classifier::AddAssign:
    return "AddAssign";
  case Classifier::SubAssign:
    return "SubAssign";
  case Classifier::Define:
    return "definition";
  case Classifier::EndStatement:
    return "end statement";
  case Classifier::AddOp:
    return "AddOp";
  case Classifier::SubOp:
    return "SubOp";
  case Classifier::DivOp:
    return "DivOp";
  case Classifier::MulOp:
    return "MulOp";
  case Classifier::ModOp:
    return "ModOp";
  case Classifier::LessOp:
    return "LessOp";
  case Classifier::GreaterOp:
    return "GreaterOp";
  case Classifier::LessEqOp:
    return "LessEqOp";
  case Classifier::GreaterEqOp:
    return "GreaterEqOp";
  case Classifier::CmpOp:
    return "CmpOp";
  case Classifier::CallOp:
    return "CallOp";
  case Classifier::AccessOp:
    return "AccessOp";
  case Classifier::AndOp:
    return "AndOp";
  case Classifier::OrOp:
    return "OrOp";
  case Classifier::NotOp:
    return "NotOp";
  case Classifier::ValidOp:
    return "ValidOp";
  case Classifier::K_if:
    return "Keyword `if`";
  case Classifier::K_for:
    return "Keyword `for`";
  case Classifier::K_else:
    return "Keyword `else`";
  case Classifier::K_return:
    return "Keyword `return`";
  case Classifier::K_func:
    return "Keyword `func`";
  case Classifier::K_type:
    return "Keyword `type`";
  case Classifier::K_import:
    return "import statement";
  case Classifier::K_from:
    return "Keyword `from`";
  case Classifier::K_true:
    return "boolean value `true`";
  case Classifier::K_false:
    return "boolean value `false`";
  case Classifier::K_library:
    return "package decleration";
  case Classifier::K_debug:
    return "Keyword `debug`";
  case Classifier::K_warning:
    return "Keyword `warning`";
  case Classifier::K_error:
    return "Keyword `error`";
  case Classifier::K_new:
    return "heap creator";
  case Classifier::K_on_load:
    return "module load function";
  case Classifier::K_init:
    return "stack creator";
  case Classifier::K_this:
    return "`this` accessor";
  case Classifier::EndOfStream:
    return "end of file";
  case Classifier::Constant:
    return "constant qualifier ([=/=])";
  case Classifier::Dynamic:
    return "dynamic qualifier ([=>>])";
  case Classifier::Hidden:
    return "hidden qualifier ([=!=])";
  case Classifier::Temporary:
    return "temporary qualifier ([***])";
  case Classifier::TOTAL_FLAGS:
  case Classifier::None:
    return "unknown";
  }
};
}  // namespace Tetrodotoxin::Language::Parser