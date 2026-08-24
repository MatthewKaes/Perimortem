// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/render/dialect.hpp"

#include "tetrodotoxin/render/archive/reader.hpp"
#include "tetrodotoxin/render/archive/writer.hpp"
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

auto Render::Dialect::encode(
    const Abstract& monograph,
    Tetrodotoxin::Language::Persistence::Profile profile) const
    -> Option<Dynamic::Bytes> {
  auto render = monograph.select<Render::Language::Monograph>();
  BAIL_IF(!render);
  return Render::Archive::Writer::encode(*render, profile);
}

auto Render::Dialect::restore(
    Allocator::Arena& arena,
    View::Bytes payload,
    Tetrodotoxin::Language::Persistence::Profile profile,
    const Documentation&,
    Abstract& context) -> Option<Tetrodotoxin::Language::Monograph&> {
  auto restored =
      Render::Archive::Reader::restore(arena, payload, profile, *this, context);
  return restored ? Option<Tetrodotoxin::Language::Monograph&>(*restored)
                  : Option<Tetrodotoxin::Language::Monograph&>();
}
