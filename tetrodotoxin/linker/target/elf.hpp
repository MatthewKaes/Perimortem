// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/access/bytes.hpp"
#include "perimortem/core/perimortem.hpp"

#include "perimortem/memory/dynamic/bytes.hpp"
#include "perimortem/memory/dynamic/vector.hpp"

#include "tetrodotoxin/linker/object/relocation.hpp"
#include "tetrodotoxin/linker/object/section.hpp"
#include "tetrodotoxin/linker/object/symbol.hpp"
#include "tetrodotoxin/linker/target/format.hpp"

namespace Tetrodotoxin::Linker::Target {

class Elf : public Format {
 public:
  Elf();

  auto add_section(Object::Section section) -> Bits_16 override;
  auto add_symbol(Object::Symbol symbol) -> void override;
  auto add_relocation(Object::Relocation relocation) -> void override;
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

  Perimortem::Memory::Dynamic::Vector<Object::Section> sections;
  Perimortem::Memory::Dynamic::Vector<Object::Symbol> symbols;
  Perimortem::Memory::Dynamic::Vector<
      Perimortem::Memory::Dynamic::Vector<Object::Relocation>>
      relocation_tables;
  Count relocation_count = 0;
  Count relocation_section_count = 0;
};

}  // namespace Tetrodotoxin::Linker::Target
