// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/language/model/addressable.hpp"

#include "tetrodotoxin/library/llvm/builder.hpp"

using namespace Tetrodotoxin::Library;

auto Language::Model::Addressable::reserve_declaration(
    Llvm::Program& program) const -> Bool {
  return get_type().reserve(program);
}

auto Language::Model::Addressable::complete_declaration(
    Llvm::Program& program) const -> Bool {
  Bool completed = get_type().complete(program);
  if (!completed) {
    return False;
  }

  auto anchor = get_declaration_anchor();
  if (!anchor) {
    return True;
  }

  return program.debug_field(*this, *anchor);
}
