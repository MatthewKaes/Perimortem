// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/puffer/isa/boot/virtual_machine.hpp"

#include "perimortem/memory/managed/vector.hpp"

#include "tetrodotoxin/isa/base/documentation.hpp"

using namespace Perimortem::Memory;
using namespace Tetrodotoxin::Puffer;
using namespace Ttx::Lexical;

auto Isa::Boot::VirtualMachine::evaluate(
    Cursor& cursor,
    const Tetrodotoxin::Isa::Registry& registry) -> Isa::Boot::Envelope* {
  const auto documentation =
      Tetrodotoxin::Isa::Base::Documentation::evaluate(cursor);
  if (!cursor.require(
          Class::Type::Dialect,
          "Expected ISA selection such as `dialect : Library`."_view)) {
    return nullptr;
  }

  if (!cursor.require(
          Class::Type::Define,
          "Expected `:` after dialect instruction."_view)) {
    return nullptr;
  }

  const Token* isa =
      cursor.require(Class::Type::Type, "Expected ISA name."_view);
  if (isa == nullptr || !registry.require_installed(cursor, *isa)) {
    return nullptr;
  }

  if (!cursor.require(
          Class::Type::EndStatement,
          "Expected `;` after dialect instruction."_view)) {
    return nullptr;
  }

  Managed::Vector<Isa::Boot::Import> imports(cursor.get_arena());
  while (cursor.matches(Class::Type::Import)) {
    auto import = Isa::Boot::Import::evaluate(cursor, registry);
    if (!import.is_valid()) {
      return nullptr;
    }

    imports.insert(import);
  }

  return &cursor.get_arena().construct<Isa::Boot::Envelope>(
      documentation, isa->get_text(), imports);
}
