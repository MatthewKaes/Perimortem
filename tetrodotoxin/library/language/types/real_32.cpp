// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/language/types/real_32.hpp"

#include "tetrodotoxin/library/language/constants/real.hpp"

using namespace Perimortem::Core;
using namespace Tetrodotoxin::Library::Language;

auto Types::Real_32::create_default(
    Perimortem::Memory::Allocator::Arena& arena) const -> Option<Model::Pack&> {
  return Constants::Real::create_synthetic(arena, *this, 0.0);
}
