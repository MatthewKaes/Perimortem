// Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/app/language/monograph.hpp"

using namespace Perimortem::Memory;
using namespace Ttx::Concept;
using namespace Ttx::Lexical;
using namespace Tetrodotoxin::App;

auto Language::Monograph::create(
    Allocator::Arena& arena,
    const Abstract& language,
    const Documentation& documentation,
    Abstract& context,
    Runtime& runtime,
    Program& program) -> Monograph& {
  return arena.construct_from<Monograph>([&]() {
    return Monograph(arena, language, documentation, context, runtime, program);
  });
}

auto Language::Monograph::link(Cursor& cursor) -> Bool {
  return program.link(cursor, context);
}

auto Language::Monograph::finalize(Cursor&) -> Bool {
  return Bool(program.get_entry());
}

auto Language::Monograph::link_restored() -> Bool {
  return program.link_restored(context);
}

auto Language::Monograph::finalize_restored() -> Bool {
  return Bool(program.get_entry());
}
