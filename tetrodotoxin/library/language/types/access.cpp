// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/language/types/access.hpp"

#include "tetrodotoxin/library/language/constants/bytes.hpp"

using namespace Perimortem::Core;
using namespace Tetrodotoxin::Library::Language;

auto Types::Access::create_default(
    Perimortem::Memory::Allocator::Arena& arena) const -> Option<Model::Pack&> {
  return Constants::Bytes::create_synthetic(arena, *this, {});
}
