// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/language/types/unsigned_32.hpp"

#include "tetrodotoxin/library/language/constants/unsigned.hpp"

using namespace Perimortem::Core;
using namespace Tetrodotoxin::Library::Language;

auto Types::Unsigned_32::create_default(
    Perimortem::Memory::Allocator::Arena& arena) const -> Option<Model::Pack&> {
  return Constants::Unsigned::create_synthetic(arena, *this, 0);
}
