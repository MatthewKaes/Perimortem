// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/render/dialect.hpp"

#include "tetrodotoxin/render/interpreter/source.hpp"
#include "tetrodotoxin/render/language/monograph.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Ttx::Concept;
using namespace Ttx::Lexical;
using namespace Tetrodotoxin;

auto Render::Dialect::interpret(
    Cursor& cursor,
    const Documentation& documentation,
    const Anchor&,
    Abstract& context) -> Option<Tetrodotoxin::Language::Monograph&> {
  auto& monograph = Render::Language::Monograph::create(
      cursor.get_arena(), *this, documentation, context);
  Render::Interpreter::Source::parse(monograph, cursor);
  return monograph;
}
