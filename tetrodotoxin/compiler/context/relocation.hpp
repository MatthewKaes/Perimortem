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

  static auto create_pc32(Count sym, Count code) -> Relocation {
    return {sym, code - 4, Type::Pc32, -4};
  }

  Count symbol;
  Count offset;
  Type type;
  SignedBits_32 addend;
};

}  // namespace Tetrodotoxin::Compiler::Context
