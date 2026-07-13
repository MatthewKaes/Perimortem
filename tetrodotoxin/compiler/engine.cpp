// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/compiler/engine.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Perimortem::Utility;
using namespace Tetrodotoxin::Compiler;

auto Engine::publish_read_only(
    View::Bytes content,
    View::Vector<Symbol> symbols) -> Bool {
  if (content.is_empty() && !symbols.is_empty()) {
    return False;
  }

  if (content.is_empty()) {
    return True;
  }

  for (Count i = 0; i < symbols.get_size(); i++) {
    Range range = symbols[i].get_range();
    if (range.start > content.get_size() ||
        range.size > content.get_size() - range.start) {
      return False;
    }
  }

  Bits_16 section = linker.add_section(
      Tetrodotoxin::Linker::Object::Section::Type::ReadOnly, content);
  for (Count i = 0; i < symbols.get_size(); i++) {
    linker.add_symbol(
        Tetrodotoxin::Linker::Object::Symbol::create_read_only(
            symbols[i].get_name(), section, symbols[i].get_range()));
  }

  return True;
}

auto Engine::build_archive(View::Bytes object_name) -> Dynamic::Bytes {
  if (object_name.is_empty() || !backend.lower(program, errors, linker)) {
    return Dynamic::Bytes();
  }

  return linker.build_library(object_name);
}

auto Engine::build_header() const -> Dynamic::Bytes {
  return backend.build_header(program);
}
