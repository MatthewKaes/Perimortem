// Tetrodotoxin
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
  // This leaf admits only the empty canonical body. Scene declarations remain
  // with their later semantic owners instead of entering a partial inventory.
  if (!cursor.matches(Code::Type::Terminal)) {
    cursor.create_token_error(
        "Scene could not interpret this declaration."_view);
    return {};
  }

  auto child = library.interpret(cursor, documentation, source_anchor, context);
  if (!child) {
    return {};
  }

  auto library_child = child->select<Library::Language::Monograph>();
  BAIL_IF(!library_child);
  Allocator::Arena& arena = cursor.get_arena();
  return arena.construct<Scene::Language::Monograph>(
      arena, documentation, *this, context, *library_child);
}
