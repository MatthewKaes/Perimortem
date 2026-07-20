// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/view/vector.hpp"

#include "perimortem/memory/dynamic/bytes.hpp"

#include "tetrodotoxin/compiler/backend.hpp"
#include "tetrodotoxin/compiler/program.hpp"
#include "tetrodotoxin/compiler/symbol.hpp"
#include "tetrodotoxin/linker/linker.hpp"
#include "ttx/lexical/errors.hpp"

namespace Tetrodotoxin::Compiler {

// Owns the native publication state for one compiler transaction. The caller
// owns the Program because Dialect lowering builds that product before a backend
// consumes it. Keeping the two lifetimes separate prevents Engine from
// becoming a mutable compiler context that later phases can use as a cache.
class Engine {
 public:
  Engine(Ttx::Lexical::Errors& errors, Backend backend)
      : errors(errors), backend(backend) {}

  auto publish_read_only(
      Perimortem::Core::View::Bytes content,
      Perimortem::Core::View::Vector<Symbol> symbols) -> Bool;
  auto build_archive(
      const Program& program,
      Perimortem::Core::View::Bytes object_name)
      -> Perimortem::Memory::Dynamic::Bytes;
  auto build_header(const Program& program) const
      -> Perimortem::Memory::Dynamic::Bytes;

 private:
  Ttx::Lexical::Errors& errors;
  Backend backend;
  Tetrodotoxin::Linker::Linker linker;
};

}  // namespace Tetrodotoxin::Compiler
