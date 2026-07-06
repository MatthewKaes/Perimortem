// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"

#include "perimortem/utility/range.hpp"

namespace Tetrodotoxin::Compiler::Symbol {

class String {
 public:
  Perimortem::Core::View::Bytes name;

  // `value` is the stable decoded text used for compiler-side deduplication.
  // `range` is the emitted location in the read-only string section that the
  // linker needs when it creates the object symbol.
  Perimortem::Core::View::Bytes value;
  Perimortem::Utility::Range range;
};

}  // namespace Tetrodotoxin::Compiler::Symbol
