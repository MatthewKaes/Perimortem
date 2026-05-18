// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/memory/allocator/arena.hpp"
#include "perimortem/memory/dynamic/bytes.hpp"
#include "perimortem/memory/managed/vector.hpp"

#include "tetrodotoxin/linker/target/format.hpp"

namespace Tetrodotoxin::Linker {

// Merges several sections and symbols into an output target used for static
// linking. Dynamic linking is not currently supported.
//
// Sections, symbols, and relocations are accumulated via add_* calls. A single
// emit_object or emit_archive call then serializes the complete binary.
//
//
class Linker {
 public:
  enum class OS {
    Linux,
    Windows,
  };

  Linker(OS target = OS::Linux);
  ~Linker();

  // Returns the 0-based index of the new section.
  auto add_section(
      Context::Symbol::Location section,
      Perimortem::Core::View::Bytes data) -> void;

  // Returns the 0-based index of the new symbol.
  auto add_symbol(InputSymbol symbol) -> Bits_32;

  auto add_relocation(InputRelocation relocation) -> void;

  // Serialize an ELF64 ET_REL object file into output.
  auto emit_object(Perimortem::Memory::Dynamic::Bytes& output) -> void;

  // Wrap an object file in an ar(1) static library.
  // object_name is the member name inside the archive (e.g. "ttx.o").
  // The ar symbol table is populated with all global symbols so that the
  // archive can be passed directly to clang/lld without running ranlib.
  auto build_library(Perimortem::Core::View::Bytes object_name)
      -> Perimortem::Memory::Dynamic::Bytes;

  // Clear all accumulated sections, symbols, and relocations.
  auto reset() -> void;

 private:
  Perimortem::Memory::Allocator::Arena arena;
  Target::Format* format;
};

// Convenience: populate a Linker from a compiled Compiler output and emit an
// ar(1) archive. object_name is the member name inside the archive (e.g.
// "ttx.o").
auto emit_archive(
    Compiler::Compiler& compiler,
    Perimortem::Core::View::Bytes object_name)
    -> Perimortem::Memory::Dynamic::Bytes;

}  // namespace Tetrodotoxin::Linker
