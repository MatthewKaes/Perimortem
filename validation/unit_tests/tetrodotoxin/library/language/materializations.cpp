// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/language/materializations.hpp"

#include "validation/unit_test.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Tetrodotoxin::Library::Language;
using namespace Validation;

static Harness LibraryMaterializations = {
  .name = "Tetrodotoxin::Library::Language::Materializations"_view,
};

PERIMORTEM_UNIT_TEST(LibraryMaterializations, inactive) {
  Allocator::Arena arena;
  Materializations materializations(arena);

  EXPECT_EQ(materializations.get_size(), Count(0));
}
