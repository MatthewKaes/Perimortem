// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/linker/linker.hpp"

#include "tetrodotoxin/compiler/compiler.hpp"
#include "tetrodotoxin/compiler/context/relocation.hpp"
#include "tetrodotoxin/compiler/context/symbol.hpp"
#include "tetrodotoxin/linker/target/elf.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Tetrodotoxin::Linker;
using namespace Tetrodotoxin::Compiler;

Linker::Linker() {
  format = new (Bibliotheca::check_out(sizeof(Target::Elf)).ptr) Target::Elf();
}
Linker::~Linker() {
  Bibliotheca::remit(Data::cast<Bits_8>(format));
}

auto Linker::add_section(Context::Symbol::Location type, View::Bytes data)
    -> void {
  format->add_section({type, data});
}

auto Linker::add_symbol(Context::Symbol symbol) -> void {
  format->add_symbol(symbol);
}

auto Linker::add_relocation(Context::Relocation relocation) -> void {
  format->add_relocation(relocation);
}

auto Linker::build_library(View::Bytes object_name) -> Dynamic::Bytes {
  return format->build_library(object_name);
}

auto Linker::reset() -> void {
  format->reset();
}

auto Tetrodotoxin::Linker::emit_archive(
    Compiler::Compiler& compiler,
    View::Bytes object_name) -> Dynamic::Bytes {
  Linker linker;

  // .text is always added so symbols with Program context resolve correctly.
  linker.add_section(Context::Symbol::Location::Program, compiler.get_code());
  if (compiler.get_strings().get_size() > 0) {
    linker.add_section(
        Context::Symbol::Location::Strings, compiler.get_strings());
  }

  // Symbols must be added in compiler order to keep relocation symbol indices
  // valid.
  const auto symbols = compiler.get_symbols();
  for (Count i = 0; i < symbols.get_size(); i++) {
    linker.add_symbol(symbols[i]);
  }

  const auto relocs = compiler.get_relocs();
  for (Count i = 0; i < relocs.get_size(); i++) {
    linker.add_relocation(relocs[i]);
  }

  return linker.build_library(object_name);
}
