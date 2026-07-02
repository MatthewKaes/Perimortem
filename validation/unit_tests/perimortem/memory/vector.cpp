// Perimortem Engine
// Copyright © Matt Kaes

#include "validation/unit_test.hpp"

#include "perimortem/core/null_terminated.hpp"

#include "perimortem/memory/dynamic/vector.hpp"

using namespace Perimortem::Memory;
using namespace Validation;

static Harness DynamicVector = {
  .name = "Dynamic::Vector"_view,
};

PERIMORTEM_UNIT_TEST(DynamicVector, remove) {
  Dynamic::Vector<Signed_32> values;
  values.insert(1);
  values.insert(2);
  values.insert(3);
  values.insert(4);

  EXPECT(values.remove(1));
  EXPECT_EQ(values.get_size(), 3);
  EXPECT_EQ(values[0], 1);
  EXPECT_EQ(values[1], 4);
  EXPECT_EQ(values[2], 3);
  EXPECT(!values.contains(2));
  EXPECT(!values.remove(3));
}

PERIMORTEM_UNIT_TEST(DynamicVector, remove_stable) {
  Dynamic::Vector<Signed_32> values;
  values.insert(1);
  values.insert(2);
  values.insert(3);
  values.insert(4);

  EXPECT(values.remove_stable(1));
  EXPECT_EQ(values.get_size(), 3);
  EXPECT_EQ(values[0], 1);
  EXPECT_EQ(values[1], 3);
  EXPECT_EQ(values[2], 4);
  EXPECT(!values.contains(2));
  EXPECT(!values.remove_stable(3));
}
