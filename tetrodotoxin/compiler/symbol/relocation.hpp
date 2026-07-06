// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/perimortem.hpp"

namespace Tetrodotoxin::Compiler::Symbol {

// Relocation describes one fixup the linker must apply to generated code.
//
// `target` selects the compiler-owned table indexed by `target_index`; the
// linker converts that compact compiler fact into the target object format's
// relocation record.
class Relocation {
 public:
  enum class Target : Bits_8 {
    String,
    External,
  };

  enum class Type : Bits_8 {
    Pc32,
    Plt32,
  };

  Target target;
  Count target_index;
  Count code_offset;
  Type type;
};

}  // namespace Tetrodotoxin::Compiler::Symbol
