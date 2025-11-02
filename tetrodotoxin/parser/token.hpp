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
  Disabled,    // </>
  String,      // " "
  Numeric,     // 0-1 no decemial
  Float,       // 9. 0.9 as floats 9.. isn't valid TODO: Handle 9...?
  Attribute,   // @ can contain breaks
  Identifier,  // Default symbols
  Type,        // Type specifier
  Parameter,   // Passed into functions.
  Constant,    // [=/=]
  Dynamic,     // [=>>]
  Hidden,      // [=!=]
  Temporary,   // [***]

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

  // Keywords
  As,
  If,
  For,
  New,
  Via,
  Else,
  Func,
  Init,
  Self,
  True,
  Alias,
  Error,  // Special function
  Debug,  // Special function
  False,
  Using,
  While,
  Entity,
  Object,
  Return,
  Struct,
  Library,
  Package,
  OnLoad,   // on_load
  Warning,  // Special function

  // Script types
  EndOfStream,

  TOTAL_FLAGS,  // Always the last flag for validating.
};

using ClassifierFlags = Perimortem::Concepts::BitFlag<Classifier>;

// Wrapper for tokenizing a stream.
struct Token {
  Classifier klass;
  const std::string_view data;
  Location location;

  inline auto test(std::string_view view) const -> bool {
    return data.size() == view.size() &&
           std::memcpy((void*)data.data(), view.data(), data.size());
  }

  inline auto valid() const -> bool { return klass != Classifier::EndOfStream; }
};

using TokenStream = std::vector<Token>;

inline constexpr auto klass_name(Classifier klass) -> const char* {
  switch (klass) {
    case Classifier::Comment:
      return "comment";
    case Classifier::Disabled:
      return "disabled";
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
    case Classifier::If:
      return "if";
    case Classifier::For:
      return "for";
    case Classifier::Else:
      return "else";
    case Classifier::While:
      return "while";
    case Classifier::Return:
      return "return";
    case Classifier::Func:
      return "function definition";
    case Classifier::Object:
      return "managed class definition";
    case Classifier::Struct:
      return "storage class definition";
    case Classifier::Library:
      return "ttx library decleration";
    case Classifier::Entity:
      return "ttx entity decleration";
    case Classifier::Using:
      return "import package statement";
    case Classifier::Via:
      return "import path specifier";
    case Classifier::True:
      return "boolean value `true`";
    case Classifier::False:
      return "boolean value `false`";
    case Classifier::Package:
      return "package decleration";
    case Classifier::Debug:
      return "Keyword `debug`";
    case Classifier::Warning:
      return "Keyword `warning`";
    case Classifier::Error:
      return "Keyword `error`";
    case Classifier::OnLoad:
      return "module load function";
    case Classifier::Init:
      return "stack creator";
    case Classifier::New:
      return "heap creator";
    case Classifier::Self:
      return "`self` accessor";
    case Classifier::As:
      return "external alias decleration";
    case Classifier::Alias:
      return "internal alias decleration";
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