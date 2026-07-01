// Perimortem Engine
// Copyright (c) Matt Kaes

#pragma once

#include "perimortem/core/perimortem.hpp"

namespace Tetrodotoxin::Linker::Object {

// Relocation is the linker-facing patch request emitted by terminal code
// generators. It records which symbol index should be patched into which object
// offset, but it does not know which frontend or lowering pass required that
// reference.
class Relocation {
 public:
  enum class Type : Bits_8 {
    Pc32,
    Plt32,
  };

  static constexpr auto create_pc32(Count symbol_index, Count code_offset)
      -> Relocation {
    return Relocation(symbol_index, code_offset - 4, Type::Pc32, -4);
  }

  constexpr auto get_symbol() const -> Count { return symbol; }
  constexpr auto get_offset() const -> Count { return offset; }
  constexpr auto get_type() const -> Type { return type; }
  constexpr auto get_addend() const -> Signed_32 { return addend; }

 private:
  constexpr Relocation(
      Count symbol,
      Count offset,
      Type type,
      Signed_32 addend)
      : symbol(symbol), offset(offset), type(type), addend(addend) {}

  Count symbol = 0;
  Count offset = 0;
  Type type = Type::Pc32;
  Signed_32 addend = 0;
};

}  // namespace Tetrodotoxin::Linker::Object
