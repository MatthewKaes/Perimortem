// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/language/types/range.hpp"

#include "tetrodotoxin/library/language/constants/range.hpp"

using namespace Perimortem::Core;
using namespace Tetrodotoxin::Library::Language;

auto Types::Range::create_default(
    Perimortem::Memory::Allocator::Arena& arena) const -> Option<Model::Pack&> {
  return Constants::Range::create_synthetic(arena, *this);
}
