// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/perimortem.hpp"

#include "perimortem/memory/allocator/arena.hpp"

namespace Tetrodotoxin::Compiler::Context {

class Names {
 public:
  Names(Perimortem::Memory::Allocator::Arena& arena) : arena(arena) {};

  // Creates a canonical export name.
  auto canonicalize(
      Perimortem::Core::View::Bytes module,
      Perimortem::Core::View::Bytes name) -> Perimortem::Core::View::Bytes;

  // Allocates a temporary name for use in symbols and internal linkage.
  auto make_local_unique() -> Perimortem::Core::View::Bytes;

 private:
  Bits_8 counter[4] = {0, 0, 0, 0};
  Perimortem::Memory::Allocator::Arena& arena;
};

}  // namespace Tetrodotoxin::Compiler::Context
