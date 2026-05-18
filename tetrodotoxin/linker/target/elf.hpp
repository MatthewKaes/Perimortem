// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/view/vector.hpp"
#include "perimortem/core/perimortem.hpp"

#include "perimortem/memory/allocator/arena.hpp"
#include "perimortem/memory/dynamic/bytes.hpp"
#include "perimortem/memory/dynamic/vector.hpp"

#include "tetrodotoxin/compiler/context/relocation.hpp"
#include "tetrodotoxin/compiler/context/symbol.hpp"
#include "tetrodotoxin/linker/target/format.hpp"

// ELF-64 constants and on-disk structures for x86-64 relocatable objects.
// All multi-byte fields are little-endian. See elf(5):
// https://man7.org/linux/man-pages/man5/elf.5.html

namespace Tetrodotoxin::Linker::Target {

class Elf : Format {
 public:
  auto build_library(Perimortem::Core::View::Bytes object_name)
      -> Perimortem::Memory::Dynamic::Bytes;

 private:
  auto build_object(
      Perimortem::Memory::Allocator::Arena& arena,
      Perimortem::Core::View::Vector<Format::Section> sections,
      Perimortem::Core::View::Vector<Compiler::Context::Symbol> symbols,
      Perimortem::Core::View::Vector<Compiler::Context::Relocation> relocations)
      -> Perimortem::Memory::Dynamic::Bytes;

  auto write_header(
      Perimortem::Core::Access::Bytes buffer,
      Bits_64 section_offset,
      Bits_16 section_count,
      Bits_16 shstrtab_index) -> void;

  auto create_elf_sections(View::Vector<Format::Section> sections);

  Dynamic::Vector<SectionProperties> elf_sections;
  Count total_symbols;
  Count str_section_index;
};

}  // namespace Tetrodotoxin::Linker::Target
