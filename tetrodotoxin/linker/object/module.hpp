// Perimortem Engine
// Copyright (c) Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/view/vector.hpp"
#include "perimortem/core/perimortem.hpp"

#include "perimortem/memory/dynamic/vector.hpp"

#include "perimortem/utility/option.hpp"

#include "tetrodotoxin/linker/object/relocation.hpp"
#include "tetrodotoxin/linker/object/section.hpp"
#include "tetrodotoxin/linker/object/symbol.hpp"

namespace Tetrodotoxin::Linker::Object {

// Module owns one ordered object inventory before target encoding. Index zero
// remains reserved for the undefined Section, while every accepted Symbol and
// Relocation is checked against the inventory already constructed before it.
class Module {
 public:
  Module();

  auto add_section(Section::Type type, Perimortem::Core::View::Bytes data)
      -> Perimortem::Utility::Option<Unsigned_16>;
  auto add_section(Section section) -> Perimortem::Utility::Option<Unsigned_16>;
  auto add_symbol(Symbol symbol) -> Perimortem::Utility::Option<Count>;
  auto add_relocation(Relocation relocation) -> Bool;

  constexpr auto get_sections() const
      -> Perimortem::Core::View::Vector<Section> {
    return sections.get_view();
  }

  constexpr auto get_symbols() const -> Perimortem::Core::View::Vector<Symbol> {
    return symbols.get_view();
  }

  constexpr auto get_relocations() const
      -> Perimortem::Core::View::Vector<Relocation> {
    return relocations.get_view();
  }

 private:
  Perimortem::Memory::Dynamic::Vector<Section> sections;
  Perimortem::Memory::Dynamic::Vector<Symbol> symbols;
  Perimortem::Memory::Dynamic::Vector<Relocation> relocations;
};

}  // namespace Tetrodotoxin::Linker::Object
