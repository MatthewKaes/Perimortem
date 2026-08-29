// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/library/dialect.hpp"

#include "tetrodotoxin/library/archive/reader.hpp"
#include "tetrodotoxin/library/archive/writer.hpp"
#include "tetrodotoxin/library/interpreter/source/library.hpp"
#include "tetrodotoxin/library/language/monograph.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Ttx::Concept;
using namespace Ttx::Lexical;
using namespace Tetrodotoxin;

auto Library::Dialect::interpret(
    Cursor& cursor,
    const Documentation& documentation,
    const Anchor& source_anchor,
    Abstract& context) -> Option<Tetrodotoxin::Language::Monograph&> {
  auto& monograph = Language::Monograph::create_authored(
      cursor.get_arena(), documentation, source_anchor, *this, context);
  Interpreter::Source::Library::parse(monograph.get_source(), cursor);
  return monograph;
}

auto Library::Dialect::encode(const Abstract& monograph) const
    -> Option<Dynamic::Bytes> {
  auto library = monograph.select<Language::Monograph>();
  BAIL_IF(!library);

  return Archive::Writer::write(*library);
}

auto Library::Dialect::restore(
    Allocator::Arena& arena,
    View::Bytes payload,
    const Documentation&,
    Abstract& context) -> Option<Tetrodotoxin::Language::Monograph&> {
  auto restored = Archive::Reader::read(arena, payload, *this, context);
  return restored ? Option<Tetrodotoxin::Language::Monograph&>(*restored)
                  : Option<Tetrodotoxin::Language::Monograph&>();
}
