// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/memory/dynamic/bytes.hpp"

#include "tetrodotoxin/compiler/execution/program.hpp"
#include "tetrodotoxin/linker/linker.hpp"
#include "ttx/lexical/errors.hpp"

namespace Tetrodotoxin::Compiler {

// Backend is the toolchain-selected continuation for an execution Program.
//
// Lowering receives semantic compiler data, diagnostics, and the linker
// transaction. The header callback independently owns the host-language
// declarations used to call the resulting symbols.
class Backend {
 public:
  using Lowerer = Bool (*)(
      const Execution::Program& program,
      Ttx::Lexical::Errors& errors,
      Tetrodotoxin::Linker::Linker& linker);
  using HeaderBuilder =
      Perimortem::Memory::Dynamic::Bytes (*)(const Execution::Program& program);

  constexpr Backend() = default;
  constexpr Backend(Lowerer lowerer, HeaderBuilder header_builder)
      : lowerer(lowerer), header_builder(header_builder) {}

  constexpr auto lower(
      const Execution::Program& program,
      Ttx::Lexical::Errors& errors,
      Tetrodotoxin::Linker::Linker& linker) const -> Bool {
    return lowerer != nullptr && lowerer(program, errors, linker);
  }

  auto build_header(const Execution::Program& program) const
      -> Perimortem::Memory::Dynamic::Bytes {
    return header_builder == nullptr ? Perimortem::Memory::Dynamic::Bytes()
                                     : header_builder(program);
  }

 private:
  Lowerer lowerer = nullptr;
  HeaderBuilder header_builder = nullptr;
};

}  // namespace Tetrodotoxin::Compiler
