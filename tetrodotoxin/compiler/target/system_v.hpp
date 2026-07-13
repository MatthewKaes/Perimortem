// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "tetrodotoxin/compiler/backend.hpp"
#include "tetrodotoxin/compiler/target/cpp.hpp"

namespace Tetrodotoxin::Compiler::Target {

// Selects x86-64 System V lowering with the C++ host declarations emitted by
// the standard toolchain.
class SystemV {
 public:
  static constexpr auto backend() -> Backend {
    return Backend(lower, Cpp::build_header);
  }

 private:
  static auto lower(
      const Execution::Program& program,
      Ttx::Lexical::Errors& errors,
      Tetrodotoxin::Linker::Linker& linker) -> Bool;
};

}  // namespace Tetrodotoxin::Compiler::Target
