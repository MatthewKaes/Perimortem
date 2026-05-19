// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/access/bytes.hpp"
#include "perimortem/core/perimortem.hpp"

#include "perimortem/memory/dynamic/bytes.hpp"
#include "perimortem/memory/dynamic/vector.hpp"

#include "tetrodotoxin/compiler/context/relocation.hpp"
#include "tetrodotoxin/compiler/context/symbol.hpp"
#include "tetrodotoxin/linker/target/format.hpp"

namespace Tetrodotoxin::Linker::Target {

class Elf : public Format {
 public:
  auto add_section(Format::Section section) -> void override;
  auto add_symbol(Compiler::Context::Symbol symbol) -> void override;
  auto add_relocation(Compiler::Context::Relocation relocation)
      -> void override;
  auto build_library(Perimortem::Core::View::Bytes object_name)
      -> Perimortem::Memory::Dynamic::Bytes override;
  auto reset() -> void override;

 private:
  auto build_object() -> Perimortem::Memory::Dynamic::Bytes;
  auto write_header(
      Perimortem::Core::Access::Bytes buffer,
      Bits_64 section_offset,
      Bits_16 section_count,
      Bits_16 section_string_table_index) -> void;

  Perimortem::Memory::Dynamic::Vector<Format::Section> sections;
  Perimortem::Memory::Dynamic::Vector<Compiler::Context::Symbol> symbols;
  Perimortem::Memory::Dynamic::Vector<Compiler::Context::Relocation>
      relocations;
};

}  // namespace Tetrodotoxin::Linker::Target
