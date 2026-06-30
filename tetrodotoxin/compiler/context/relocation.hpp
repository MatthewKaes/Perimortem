// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/perimortem.hpp"

namespace Tetrodotoxin::Compiler::Context {

class Relocation {
 public:
  enum class Type {
    Pc32,
    Plt32,
  };

  static auto create_pc32(Count symbol_index, Count code_offset) -> Relocation {
    return {symbol_index, code_offset - 4, Type::Pc32, -4};
  }

  Count symbol;
  Count offset;
  Type type;
  Signed_32 addend;
};

}  // namespace Tetrodotoxin::Compiler::Context
