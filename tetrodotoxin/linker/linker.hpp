// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/perimortem.hpp"

#include "perimortem/memory/dynamic/bytes.hpp"
#include "perimortem/memory/dynamic/object.hpp"
#include "perimortem/memory/dynamic/vector.hpp"

#include "tetrodotoxin/linker/object/relocation.hpp"
#include "tetrodotoxin/linker/object/section.hpp"
#include "tetrodotoxin/linker/object/symbol.hpp"
#include "tetrodotoxin/linker/target/elf.hpp"

namespace Tetrodotoxin::Linker {

class Linker {
 public:
  Linker() = default;

  auto add_section(
      Object::Section::Type type,
      Perimortem::Core::View::Bytes data) -> Bits_16;
  auto add_section(Object::Section section) -> Bits_16;

  auto add_symbol(Object::Symbol symbol) -> Count;
  auto add_relocation(Object::Relocation relocation) -> void;

  auto build_library(Perimortem::Core::View::Bytes object_name)
      -> Perimortem::Memory::Dynamic::Bytes;

  auto reset() -> void;

 private:
  Target::Elf format;
  Perimortem::Memory::Dynamic::Vector<
      Perimortem::Memory::Dynamic::Object<Perimortem::Memory::Dynamic::Bytes>>
      section_data;
  Count symbol_count = 0;
};

}  // namespace Tetrodotoxin::Linker
