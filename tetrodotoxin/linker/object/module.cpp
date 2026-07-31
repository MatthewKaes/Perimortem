// Perimortem Engine
// Copyright (c) Matt Kaes

#include "tetrodotoxin/linker/object/module.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Utility;
using namespace Tetrodotoxin::Linker;

Object::Module::Module() {
  sections.emplace(Object::Section::undefined());
}

auto Object::Module::add_section(Object::Section::Type type, View::Bytes data)
    -> Option<Unsigned_16> {
  return add_section(Object::Section(type, data));
}

auto Object::Module::add_section(Object::Section section)
    -> Option<Unsigned_16> {
  const Bool construction_has_advanced =
      symbols.get_size() != 0 || relocations.get_size() != 0;
  const Bool index_overflows = sections.get_size() > Count(Unsigned_16(-1));
  if (section.get_type() == Object::Section::Type::Undefined ||
      construction_has_advanced || index_overflows) {
    return {};
  }

  const Unsigned_16 index = Unsigned_16(sections.get_size());
  sections.emplace(Data::take(section));
  return index;
}

auto Object::Module::add_symbol(Object::Symbol symbol) -> Option<Count> {
  const Unsigned_16 section_index = symbol.get_section_index();
  const Bool existing_section =
      section_index != 0 && Count(section_index) < sections.get_size();
  const Bool valid_definition = symbol.is_defined() == existing_section;
  if (relocations.get_size() != 0 || !valid_definition) {
    return {};
  }

  const Count index = symbols.get_size();
  symbols.emplace(Data::take(symbol));
  return index;
}

auto Object::Module::add_relocation(Object::Relocation relocation) -> Bool {
  const Unsigned_16 section_index = relocation.get_section_index();
  const Bool existing_section =
      section_index != 0 && Count(section_index) < sections.get_size();
  const Bool existing_symbol = relocation.get_symbol() < symbols.get_size();
  if (!existing_section || !existing_symbol) {
    return False;
  }

  relocations.insert(relocation);
  return True;
}
