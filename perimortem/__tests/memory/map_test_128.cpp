// Perimortem Engine
// Copyright © Matt Kaes

#include "validation/test/test.hpp"

#include "perimortem/core/view/null_terminated.hpp"
#include "perimortem/memory/dynamic/bytes.hpp"
#include "perimortem/memory/dynamic/map.hpp"
#include "perimortem/memory/static/bytes.hpp"

#include "perimortem/__tests/memory/hashable.hpp"

using namespace Perimortem::Memory;
using namespace Perimortem::Utility;

using namespace Validation;

constexpr auto optimize_size = True;

Test::Harness DynamicMap128 = {.name = "Dynamic::Map::128", .setup = []() {
                              default_construct_count = 0;
                              default_destruct_count = 0;
                            }};


PERIMORTEM_UNIT_TEST(DynamicMap128, empty) {
  Dynamic::Map<Int, Int, optimize_size> empty_map;

  // Two maps fit in a cache line.
  EXPECT_EQ(sizeof(empty_map), 32);
  EXPECT_EQ(empty_map.get_size(), 0);

  // Empty maps should consume no memory and should fetch memory lazily unless
  // initial capacity is requested.
  EXPECT_EQ(empty_map.get_memory_consumption(), 0);
}

PERIMORTEM_UNIT_TEST(DynamicMap128, simple_construction) {
  Dynamic::Map<Int, Int, optimize_size> int_map = {{{1, 2}, {2, 3}, {4, 5}}};

  EXPECT_EQ(int_map.get_size(), 3);
  ASSERT_EQ(int_map[1], 2);
  EXPECT_EQ(int_map[2], 3);
  EXPECT_EQ(int_map[4], 5);
}

PERIMORTEM_UNIT_TEST(DynamicMap128, insert_on_index) {
  Dynamic::Map<Int, Int, optimize_size> empty_map;

  // Populate defaults
  for (Int i = 0; i < 10; i++) {
    empty_map[i];
  }

  EXPECT_EQ(empty_map.get_size(), 10);
  for (Int i = 0; i < 10; i++) {
    EXPECT(empty_map.contains(i));
  }
}

PERIMORTEM_UNIT_TEST(DynamicMap128, duplicate_keys) {
  Dynamic::Map<Int, Int, optimize_size> int_map = {{{1, 2}, {1, 4}}};

  EXPECT_EQ(int_map.get_size(), 1);
  ASSERT_EQ(int_map[1], 4);

  int_map.insert(2, 8);
  int_map.insert(1, 8);
  ASSERT_EQ(int_map[1], 8);
  EXPECT_EQ(int_map[2], 8);
}

PERIMORTEM_UNIT_TEST(DynamicMap128, empty_keys) {
  Dynamic::Map<Dynamic::Bytes, Int, optimize_size> empty_map;

  Int i = 0;
  while (i < 1000) {
    i++;
    empty_map.insert(Dynamic::Bytes(), i);
  }

  // Map should contain a single key which maps to the last value used.
  EXPECT_EQ(empty_map.get_size(), 1);
  EXPECT_EQ(empty_map[Dynamic::Bytes()], i);
}

PERIMORTEM_UNIT_TEST(DynamicMap128, insert_stress_test) {
  Dynamic::Map<Int, Int, optimize_size> large_map;

  for (Int i = 0; i < 1000; i++) {
    large_map.insert(i, i + 2);
  }

  EXPECT_EQ(large_map.get_size(), 1000);
  for (Int i = 0; i < 1000; i++) {
    ASSERT_EQ(large_map[i], i + 2);
  }

  EXPECT_EQ(large_map.get_memory_consumption(), 1 << 16);
}

PERIMORTEM_UNIT_TEST(DynamicMap128, capacity_stress_test) {
  Dynamic::Map<Int, Int, optimize_size> large_map;

  large_map.ensure_capacity(1000);
  for (Int i = 0; i < 1000; i++) {
    large_map.insert(i, i + 2);
  }

  EXPECT_EQ(large_map.get_size(), 1000);
  for (Int i = 0; i < 1000; i++) {
    ASSERT_EQ(large_map[i], i + 2);
  }

  EXPECT_EQ(large_map.get_memory_consumption(), 1 << 16);
}

PERIMORTEM_UNIT_TEST(DynamicMap128, key_construction_count) {
  Count construct_count = 0;
  Count destruct_count = 0;

  {
    Dynamic::Map<Hashable, Int, optimize_size> custom_map;

    for (Int i = 0; i < 100; i++) {
      custom_map.insert(Hashable(i, construct_count, destruct_count), i);
    }

    EXPECT_EQ(custom_map.get_size(), 100);
    for (Int i = 0; i < 100; i++) {
      ASSERT_EQ(custom_map[Hashable(i, construct_count, destruct_count)], i);
    }
  }

  EXPECT_EQ(construct_count, 300);
  EXPECT_EQ(construct_count, destruct_count);
  EXPECT_EQ(default_construct_count, 0);
  EXPECT_EQ(default_destruct_count, 0);
}

PERIMORTEM_UNIT_TEST(DynamicMap128, value_construction_count) {
  Count construct_count = 0;
  Count destruct_count = 0;

  {
    Dynamic::Map<Int, Hashable, optimize_size> custom_map;

    for (Int i = 0; i < 100; i++) {
      custom_map.insert(i, Hashable(i, construct_count, destruct_count));
    }

    EXPECT_EQ(custom_map.get_size(), 100);
    for (Int i = 0; i < 100; i++) {
      ASSERT(custom_map[i] == Hashable(i, construct_count, destruct_count));
    }
  }

  EXPECT_EQ(construct_count, 300);
  EXPECT_EQ(construct_count, destruct_count);
  EXPECT_EQ(default_construct_count, 0);
  EXPECT_EQ(default_destruct_count, 0);
}

PERIMORTEM_UNIT_TEST(DynamicMap128, emplace_construction_count) {
  Count construct_count = 0;
  Count destruct_count = 0;

  {
    Dynamic::Map<Int, Hashable, optimize_size> custom_map;

    for (Int i = 0; i < 100; i++) {
      custom_map.emplace(static_cast<Int&&>(i),
                         Hashable(i, construct_count, destruct_count));
    }

    EXPECT_EQ(custom_map.get_size(), 100);
    for (Int i = 0; i < 100; i++) {
      ASSERT(custom_map[i] == Hashable(i, construct_count, destruct_count));
    }
  }

  EXPECT_EQ(construct_count, 300);
  EXPECT_EQ(construct_count, destruct_count);
  EXPECT_EQ(default_construct_count, 0);
  EXPECT_EQ(default_destruct_count, 0);
}

PERIMORTEM_UNIT_TEST(DynamicMap128, dynamic_keys) {
  Dynamic::Map<Dynamic::Bytes, Int, optimize_size> text_map;

  text_map["Hello"_view] = 0;
  text_map["World"_view] = 1;

  // Byte keys
  Dynamic::Bytes text;
  text.append('a');
  for (Byte ch = 'A'; ch < 'z'; ch++) {
    text.get_access()[0] = ch;
    text_map[text] = 2 + ch;
  }

  text_map["Longer test string"_view] = 2;

  ASSERT_EQ(text_map["Hello"_view], 0);
  ASSERT_EQ(text_map["World"_view], 1);
  ASSERT_EQ(text_map["Longer test string"_view], 2);
  for (Byte ch = 'A'; ch < 'z'; ch++) {
    text.get_access()[0] = ch;
    ASSERT_EQ(text_map[text], 2 + ch);
  }
  ASSERT_EQ(text_map["Longer test string"_view], 2);
}

PERIMORTEM_UNIT_TEST(DynamicMap128, dynamic_value) {
  Dynamic::Map<Int, Dynamic::Bytes, optimize_size> text_map;

  text_map[0] = "Hello"_view;
  text_map[1] = "World"_view;
  text_map[2] = "Longer test string"_view;

  ASSERT_TEXT(text_map[0].get_view(), "Hello"_view);
  ASSERT_TEXT(text_map[1].get_view(), "World"_view);
  ASSERT_TEXT(text_map[2].get_view(), "Longer test string"_view);
}

PERIMORTEM_UNIT_TEST(DynamicMap128, size) {
  Dynamic::Map<Int, Int, optimize_size> empty_map;
  EXPECT_EQ(sizeof(empty_map), 32);
  EXPECT_EQ(empty_map.get_capacity(), 0);
  empty_map.ensure_capacity(10);
  EXPECT_EQ(empty_map.get_capacity(), 16);
  EXPECT_EQ(empty_map.get_memory_consumption(), 512);
  empty_map.ensure_capacity(100);
  EXPECT_EQ(empty_map.get_capacity(), 128);
  EXPECT_EQ(empty_map.get_memory_consumption(), 4096);
}

PERIMORTEM_UNIT_TEST(DynamicMap128, reuse) {
  Dynamic::Map<Int, Int, optimize_size> reuse_map;

  for (Int loops = 0; loops < 5; loops++) {
    reuse_map.reset();
    ASSERT_EQ(reuse_map.get_size(), 0);

    for (Int i = 0; i < 100; i++) {
      reuse_map[i] = i;
    }

    ASSERT_EQ(reuse_map.get_size(), 100);
    for (Int i = 0; i < 100; i++) {
      ASSERT_EQ(reuse_map[i], i);
    }
  }
}

PERIMORTEM_UNIT_TEST(DynamicMap128, leak_test) {
  auto pre_test_memory = Allocator::Bibliotheca::allocated_memory();

  {
    Dynamic::Map<Dynamic::Bytes, Dynamic::Bytes, optimize_size> memory_intensive;
    Dynamic::Bytes source;

    for (Int i = 0; i < 100; i++) {
      source.append('A');
      memory_intensive[source] = "Test text to copy"_view;
    }

    ASSERT_EQ(memory_intensive.get_size(), 100);
  }

  {
    Dynamic::Map<Int, Int, optimize_size> large_map;

    for (Int i = 0; i < 1000; i++) {
      large_map.insert(i, i + 2);
    }
  }

  auto post_test_memory = Allocator::Bibliotheca::allocated_memory();
  EXPECT_EQ(pre_test_memory, post_test_memory);
}
