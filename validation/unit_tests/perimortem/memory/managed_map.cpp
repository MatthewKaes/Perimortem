// Perimortem Engine
// Copyright © Matt Kaes

#include "validation/unit_test.hpp"

#include "perimortem/core/null_terminated.hpp"

#include "perimortem/memory/allocator/arena.hpp"
#include "perimortem/memory/managed/map.hpp"

using namespace Perimortem::Memory;
using namespace Validation;

static Harness ManagedMap = {
  .name = "Managed::Map"_view,
};

PERIMORTEM_UNIT_TEST(ManagedMap, empty) {
  Allocator::Arena arena;
  Managed::Map<Signed_32, Signed_32> values(arena);

  EXPECT(values.is_empty());
  EXPECT_EQ(values.get_size(), Count(0));
  EXPECT_EQ(values.get_capacity(), Count(0));
  EXPECT(values.find(4) == nullptr);
}

PERIMORTEM_UNIT_TEST(ManagedMap, simple_insert) {
  Allocator::Arena arena;
  Managed::Map<Signed_32, Signed_32> values(arena);

  values.insert(1, 2);
  values.insert(2, 3);
  values.insert(4, 5);

  EXPECT_EQ(values.get_size(), Count(3));
  EXPECT_EQ(values[1], 2);
  EXPECT_EQ(values[2], 3);
  EXPECT_EQ(values[4], 5);
}

PERIMORTEM_UNIT_TEST(ManagedMap, duplicate_keys) {
  Allocator::Arena arena;
  Managed::Map<Signed_32, Signed_32> values(arena);

  values.insert(1, 2);
  values.insert(1, 4);
  values.insert(2, 8);

  EXPECT_EQ(values.get_size(), Count(2));
  EXPECT_EQ(values[1], 4);
  EXPECT_EQ(values[2], 8);
}

PERIMORTEM_UNIT_TEST(ManagedMap, text_keys) {
  Allocator::Arena arena;
  Managed::Map<Perimortem::Core::View::Bytes, Signed_32> values(arena);

  values["Hello"_view] = 1;
  values["World"_view] = 2;
  values["Longer test string"_view] = 3;

  EXPECT(values.contains("Hello"_view));
  EXPECT_EQ(values["Hello"_view], 1);
  EXPECT_EQ(values["World"_view], 2);
  EXPECT_EQ(values["Longer test string"_view], 3);
}

PERIMORTEM_UNIT_TEST(ManagedMap, clear) {
  Allocator::Arena arena;
  Managed::Map<Signed_32, Signed_32> values(arena);

  values.insert(1, 2);
  values.insert(2, 3);
  values.clear();

  EXPECT(values.is_empty());
  EXPECT(!values.contains(1));
  values.insert(1, 4);
  EXPECT_EQ(values[1], 4);
}

PERIMORTEM_UNIT_TEST(ManagedMap, insert_stress_test) {
  Allocator::Arena arena;
  Managed::Map<Signed_32, Signed_32> values(arena);
  for (Count i = 0; i < 1000; i++) {
    values.insert(i, i + 2);
  }

  EXPECT_EQ(values.get_size(), Count(1000));
  for (Count i = 0; i < 1000; i++) {
    ASSERT_EQ(values[i], i + 2);
  }
}
