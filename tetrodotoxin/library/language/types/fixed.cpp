// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/language/types/fixed.hpp"

#include "perimortem/memory/managed/vector.hpp"

#include "tetrodotoxin/library/language/expressions/initializer.hpp"
#include "ttx/concept/reference.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Ttx::Concept;
using namespace Tetrodotoxin::Library::Language;

auto Types::Fixed::create_default(Allocator::Arena& arena) const
    -> Option<Model::Pack&> {
  BAIL_IF(get_extent() == 0 || get_extent() > Unsigned_64(Count(-1)));

  Managed::Vector<Reference<Model::Pack>> values(arena);
  values.reset(Count(get_extent()));
  for (Count index = 0; index < Count(get_extent()); index++) {
    auto value = get_element_type().create_default(arena);
    BAIL_IF(!value);
    values.insert(*value);
  }

  return Expressions::Initializer::create_synthetic(
      arena, *this, values.get_view());
}
