// Perimortem Engine
// Copyright © Matt Kaes

#include "validation/unit_test.hpp"

#include "perimortem/memory/dynamic/bytes.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Validation;

static Harness DynamicBytes = {
  .name = "Dynamic::Bytes"_view,
};

PERIMORTEM_UNIT_TEST(DynamicBytes, value_bounds) {
  Dynamic::Bytes bytes("abc"_view);

  EXPECT_EQ(bytes[2], Unsigned_8('c'));
  EXPECT_EQ(bytes[3], Unsigned_8(0));
  EXPECT_EQ(bytes.at(Count(-1)), Unsigned_8(0));
  EXPECT_EQ(Dynamic::Bytes()[0], Unsigned_8(0));
}
