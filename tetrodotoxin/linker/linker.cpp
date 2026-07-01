// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/linker/linker.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Tetrodotoxin::Linker;

auto Linker::add_section(Object::Section::Type type, View::Bytes data)
    -> void {
  format.add_section({type, data});
}

auto Linker::add_section(Object::Section section) -> void {
  format.add_section(section);
}

auto Linker::add_symbol(Object::Symbol symbol) -> void {
  format.add_symbol(symbol);
}

auto Linker::add_relocation(Object::Relocation relocation) -> void {
  format.add_relocation(relocation);
}

auto Linker::build_library(View::Bytes object_name) -> Dynamic::Bytes {
  return format.build_library(object_name);
}

auto Linker::reset() -> void {
  format.reset();
}
