// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/perimortem.hpp"

namespace Tetrodotoxin::Library::Llvm {

// Target selects the physical platform contract for one compilation request.
enum class Target : Unsigned_8 {
  X86_64SysV,
};

}  // namespace Tetrodotoxin::Library::Llvm
