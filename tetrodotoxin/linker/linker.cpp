// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/linker/linker.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Tetrodotoxin::Linker;

auto Linker::add_section(Object::Section::Type type, View::Bytes data)
    -> Unsigned_16 {
  Dynamic::Object<Dynamic::Bytes> owned(data);
  section_data.insert(owned);
  return format.add_section({type, owned->get_view()});
}

auto Linker::add_section(Object::Section section) -> Unsigned_16 {
  return add_section(section.get_type(), section.get_data());
}

auto Linker::add_symbol(Object::Symbol symbol) -> Count {
  Count index = symbol_count;
  format.add_symbol(symbol);
  symbol_count++;
  return index;
}

auto Linker::add_relocation(Object::Relocation relocation) -> void {
  format.add_relocation(relocation);
}

auto Linker::build_library(View::Bytes object_name) -> Dynamic::Bytes {
  return format.build_library(object_name);
}

auto Linker::reset() -> void {
  format.reset();
  section_data.clear();
  symbol_count = 0;
}
