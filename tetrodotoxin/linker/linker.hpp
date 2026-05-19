// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/perimortem.hpp"

#include "perimortem/memory/dynamic/bytes.hpp"

#include "tetrodotoxin/compiler/context/relocation.hpp"
#include "tetrodotoxin/compiler/context/symbol.hpp"
#include "tetrodotoxin/linker/target/format.hpp"

namespace Tetrodotoxin::Compiler {
class Compiler;
}

namespace Tetrodotoxin::Linker {

class Linker {
 public:
  Linker();
  ~Linker();

  auto add_section(
      Compiler::Context::Symbol::Location type,
      Perimortem::Core::View::Bytes data) -> void;

  auto add_symbol(Compiler::Context::Symbol symbol) -> void;
  auto add_relocation(Compiler::Context::Relocation relocation) -> void;

  auto build_library(Perimortem::Core::View::Bytes object_name)
      -> Perimortem::Memory::Dynamic::Bytes;

  auto reset() -> void;

 private:
  Target::Format* format;
};

// Convenience: populate a Linker from Compiler output and emit an ar(1)
// archive.
auto emit_archive(
    Compiler::Compiler& compiler,
    Perimortem::Core::View::Bytes object_name)
    -> Perimortem::Memory::Dynamic::Bytes;

}  // namespace Tetrodotoxin::Linker
