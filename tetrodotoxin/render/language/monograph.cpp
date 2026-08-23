// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/render/language/monograph.hpp"

using namespace Perimortem::Memory;
using namespace Ttx::Concept;
using namespace Tetrodotoxin::Render;

auto Language::Monograph::create(
    Allocator::Arena& arena,
    const Abstract& language,
    const Documentation& documentation,
    Abstract& context) -> Monograph& {
  return arena.construct_from<Monograph>(
      [&]() { return Monograph(arena, language, documentation, context); });
}
