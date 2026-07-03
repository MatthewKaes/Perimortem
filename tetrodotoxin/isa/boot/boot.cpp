// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/isa/boot/boot.hpp"

#include "perimortem/memory/managed/vector.hpp"

#include "tetrodotoxin/isa/boot/documentation.hpp"
#include "tetrodotoxin/isa/boot/import.hpp"
#include "tetrodotoxin/isa/boot/selection.hpp"

using namespace Perimortem::Memory;
using namespace Tetrodotoxin::Isa;
using namespace Ttx::Lexical;

auto Boot::evaluate(Cursor& cursor, const Registry& registry) -> Boot* {
  const auto documentation = Documentation::evaluate(cursor);

  if (!cursor.require(
          Class::Type::Dialect,
          "Expected ISA selection such as `dialect : Library`."_view)) {
    return nullptr;
  }

  const auto isa = Selection::evaluate(cursor, registry);
  if (!isa.is_valid()) {
    return nullptr;
  }

  if (!cursor.require(
          Class::Type::EndStatement,
          "Expected `;` after dialect instruction."_view)) {
    return nullptr;
  }

  Managed::Vector<Import> imports(cursor.get_arena());
  while (cursor.matches(Class::Type::Import)) {
    auto import = Import::evaluate(cursor, registry);
    if (!import.is_valid()) {
      return nullptr;
    }

    imports.insert(import);
  }

  return &cursor.get_arena().construct<Boot>(documentation, isa, imports);
}
