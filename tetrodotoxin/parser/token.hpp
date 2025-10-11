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
  K_game_object,

  K_debug,    // Special function
  K_warning,  // Special function
  K_error,    // Special function

  EndOfStream,

  _PERIMORTEM_ENABLE_BITFLAG(Classifier, uint64_t)
};

auto klass_name(Classifier klass) -> std::string_view;

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
}  // namespace Tetrodotoxin::Language::Parser