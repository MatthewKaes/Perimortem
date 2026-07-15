// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "tetrodotoxin/compiler/backend.hpp"

namespace Tetrodotoxin::Compiler::Target {

// Selects x86-64 System V lowering with the C++ host declarations emitted by
// the standard toolchain.
class SystemV {
 public:
  static auto backend() -> Backend;
};

}  // namespace Tetrodotoxin::Compiler::Target
