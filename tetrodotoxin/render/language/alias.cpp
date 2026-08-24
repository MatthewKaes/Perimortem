// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/render/language/alias.hpp"

using namespace Perimortem::Memory;
using namespace Ttx::Concept;
using namespace Ttx::Lexical;
using namespace Tetrodotoxin::Render;

auto Language::Alias::create(
    Allocator::Arena& domain,
    Tetrodotoxin::Language::Definition& definition,
    Tetrodotoxin::Language::TypeReference target) -> Alias& {
  return domain.construct_from<Alias>(
      [&]() { return Alias(definition, target); });
}

auto Language::Alias::link(Cursor& cursor, const Abstract& context) -> Bool {
  auto selected = target.resolve(cursor, context);
  return selected && bind_target(*selected);
}

auto Language::Alias::link_restored(const Abstract& context) -> Bool {
  auto selected = target.resolve_restored(context);
  return selected && bind_target(*selected);
}
