// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/scene/dialect.hpp"

#include "tetrodotoxin/scene/language/monograph.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Ttx::Concept;
using namespace Ttx::Lexical;
using namespace Tetrodotoxin;

auto Scene::Dialect::interpret(
    Cursor& cursor,
    const Documentation& documentation,
    const Anchor& source_anchor,
    Abstract& context) -> Option<Tetrodotoxin::Language::Monograph&> {
  // Library owns the ordinary declarations and execution language used by a
  // Scene. Asking the installed child Dialect to interpret the same Cursor
  // keeps those objects in its real model while Scene adds only its extensions.
  auto child = library.interpret(cursor, documentation, source_anchor, context);
  if (!child) {
    return {};
  }

  auto library_child = child->select<Library::Language::Monograph>();
  BAIL_IF(!library_child);
  Allocator::Arena& arena = cursor.get_arena();
  auto& monograph = arena.construct<Scene::Language::Monograph>(
      arena, documentation, *this, context, *library_child);
  return monograph;
}
