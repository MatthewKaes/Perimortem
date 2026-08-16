// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/language/types/signed_32.hpp"

#include "tetrodotoxin/library/language/constants/signed.hpp"

using namespace Perimortem::Core;
using namespace Tetrodotoxin::Library::Language;

auto Types::Signed_32::create_default(
    Perimortem::Memory::Allocator::Arena& arena) const -> Option<Model::Pack&> {
  return Constants::Signed::create_synthetic(arena, *this, 0);
}
