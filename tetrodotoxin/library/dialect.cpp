// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/dialect.hpp"

#include "tetrodotoxin/library/archive/reader.hpp"
#include "tetrodotoxin/library/archive/writer.hpp"
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
      cursor, documentation, source_anchor, *this, context);
  BAIL_IF(!monograph.parse(cursor));
  return monograph;
}

auto Library::Dialect::encode(
    const Abstract& monograph,
    Tetrodotoxin::Language::Persistence::Profile profile) const
    -> Option<Dynamic::Bytes> {
  auto library = monograph.select<Language::Monograph>();
  BAIL_IF(!library);

  Archive::Writer writer(profile);
  BAIL_IF(!library->persist(writer));
  return writer.take();
}

auto Library::Dialect::restore(
    Allocator::Arena& arena,
    View::Bytes payload,
    Tetrodotoxin::Language::Persistence::Profile profile,
    const Documentation&,
    Abstract& context) -> Option<Tetrodotoxin::Language::Monograph&> {
  auto reader = Archive::Reader::open(payload, profile);
  BAIL_IF(!reader);

  auto restored =
      Language::Monograph::restore(*reader, arena, profile, *this, context);
  return restored ? Option<Tetrodotoxin::Language::Monograph&>(*restored)
                  : Option<Tetrodotoxin::Language::Monograph&>();
}
