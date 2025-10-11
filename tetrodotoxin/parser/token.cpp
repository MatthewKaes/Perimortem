// Perimortem Engine
// Copyright © Matt Kaes

#include "token.hpp"

#include <cmath>
#include <cstring>
#include <iomanip>
#include <sstream>

namespace Tetrodotoxin::Language::Parser {

auto klass_name(Classifier klass) -> std::string_view {
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
    return "Keyword `library`";
  case Classifier::K_game_object:
    return "Keyword `game_object`";
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
  case Classifier::_ENABLE_BITFLAG:
  case Classifier::_MAX_BITFLAG:
  case Classifier::None:
    return "unknown";
  }
}

} // namespace Tetrodotoxin::Language::Parser