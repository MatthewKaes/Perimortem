// Perimortem Engine
// Copyright © Matt Kaes

#include "validation/test/test.hpp"

#include "perimortem/memory/dynamic/map.hpp"
#include "perimortem/memory/static/map.hpp"

using namespace Perimortem::Memory;

using namespace Validation;

Test::Harness DynamicMap = {"Dynamic::Map"};

PERIMORTEM_UNIT_TEST(DynamicMap, simple_construction) {
  Dynamic::Map<Int, Int> ints = {{{1, 2}, {2, 3}, {4, 5}}};

  EXPECT_EQ(ints.get_size(), 3);
  ASSERT_EQ(ints[1], 2);
  EXPECT_EQ(ints[2], 3);
  EXPECT_EQ(ints[4], 5);
}

PERIMORTEM_UNIT_TEST(DynamicMap, duplicate_values) {
  Dynamic::Map<Int, Int> ints = {{{1, 2}, {1, 4}}};

  EXPECT_EQ(ints.get_size(), 1);
  ASSERT_EQ(ints[1], 4);

  ints.insert(2, 8);
  ints.insert(1, 8);
  ASSERT_EQ(ints[1], 8);
  EXPECT_EQ(ints[2], 8);
}

PERIMORTEM_UNIT_TEST(DynamicMap, insert_1000) {
  Dynamic::Map<Int, Int> large_map;

  for (Int i = 0; i < 1000; i++) {
    large_map.insert(i, i + 2);
  }

  EXPECT_EQ(large_map.get_size(), 1000);
  for (Int i = 0; i < 1000; i++) {
    ASSERT_EQ(large_map[i], i + 2);
  }

  EXPECT_EQ(large_map.get_memory_consumption(),1 << 16);
}