// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/view/vector.hpp"

#include "perimortem/memory/dynamic/bytes.hpp"

#include "tetrodotoxin/compiler/backend.hpp"
#include "tetrodotoxin/compiler/execution/program.hpp"
#include "tetrodotoxin/compiler/symbol.hpp"
#include "tetrodotoxin/linker/linker.hpp"
#include "ttx/lexical/errors.hpp"

namespace Tetrodotoxin::Compiler {

// Collects compiler inputs and hands them to the selected backend.
class Engine {
 public:
  Engine(Ttx::Lexical::Errors& errors, Backend backend)
      : errors(errors), backend(backend) {}

  constexpr auto get_program() -> Execution::Program& { return program; }
  auto publish_read_only(
      Perimortem::Core::View::Bytes content,
      Perimortem::Core::View::Vector<Symbol> symbols) -> Bool;
  auto build_archive(Perimortem::Core::View::Bytes object_name)
      -> Perimortem::Memory::Dynamic::Bytes;
  auto build_header() const -> Perimortem::Memory::Dynamic::Bytes;

 private:
  Ttx::Lexical::Errors& errors;
  Backend backend;
  Execution::Program program;
  Tetrodotoxin::Linker::Linker linker;
};

}  // namespace Tetrodotoxin::Compiler
