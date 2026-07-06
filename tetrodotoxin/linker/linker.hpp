// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/perimortem.hpp"
#include "perimortem/core/view/bytes.hpp"

#include "perimortem/memory/dynamic/bytes.hpp"

#include "tetrodotoxin/linker/target/elf.hpp"
#include "tetrodotoxin/linker/object/relocation.hpp"
#include "tetrodotoxin/linker/object/section.hpp"
#include "tetrodotoxin/linker/object/symbol.hpp"

namespace Tetrodotoxin::Compiler {
class Library;
class Shader;
}

namespace Tetrodotoxin::Linker {

class Linker {
 public:
  Linker() = default;

  auto add(const Tetrodotoxin::Compiler::Library& library) -> void;
  auto add(const Tetrodotoxin::Compiler::Shader& shader) -> void;

  auto add_section(Object::Section::Type type, Perimortem::Core::View::Bytes data)
      -> Bits_16;
  auto add_section(Object::Section section) -> Bits_16;

  auto add_symbol(Object::Symbol symbol) -> void;
  auto add_relocation(Object::Relocation relocation) -> void;

  auto build_library(Perimortem::Core::View::Bytes object_name)
      -> Perimortem::Memory::Dynamic::Bytes;
  auto append_header(
      Perimortem::Memory::Dynamic::Bytes& header,
      const Tetrodotoxin::Compiler::Library& library) const -> void;

  auto reset() -> void;

 private:
  Target::Elf format;
  Count symbol_count = 0;
};

}  // namespace Tetrodotoxin::Linker
