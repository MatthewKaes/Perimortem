// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/memory/dynamic/bytes.hpp"

#include "tetrodotoxin/compiler/execution/program.hpp"

namespace Tetrodotoxin::Compiler::Target {

// Emits the C++ declarations used to call compiled TTX functions. Linkage
// symbols remain unmangled while TTX types provide their authored C++ spelling
// through the `cpp` attribute.
class Cpp {
 public:
  static auto build_header(const Execution::Program& program)
      -> Perimortem::Memory::Dynamic::Bytes;
};

}  // namespace Tetrodotoxin::Compiler::Target
