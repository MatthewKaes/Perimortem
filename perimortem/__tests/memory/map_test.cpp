// Perimortem Engine
// Copyright © Matt Kaes

#include "validation/test/test.hpp"

#include "perimortem/core/view/null_terminated.hpp"
#include "perimortem/memory/dynamic/bytes.hpp"
#include "perimortem/memory/dynamic/map.hpp"
#include "perimortem/memory/static/bytes.hpp"

using namespace Perimortem::Memory;
using namespace Perimortem::Utility;

using namespace Validation;

static Count default_construct_count = 0;
static Count default_destruct_count = 0;

Test::Harness DynamicMap = {.name = "Dynamic::Map", .setup = []() {
                              default_construct_count = 0;
                              default_destruct_count = 0;
                            }};

class Hashable {
 public:
  Hashable()
      : id(0),
        construct_count(default_construct_count),
        destruct_count(default_destruct_count) {}
  Hashable(Int id, Count& construct_count, Count& destruct_count)
      : id(id),
        construct_count(construct_count),
        destruct_count(destruct_count) {
    this->construct_count++;
  }
  Hashable(const Hashable& rhs)
      : id(rhs.id),
        construct_count(rhs.construct_count),
        destruct_count(rhs.destruct_count) {
    this->construct_count++;
  }
  Hashable(const Hashable&& rhs)
      : id(rhs.id),
        construct_count(rhs.construct_count),
        destruct_count(rhs.destruct_count) {
    this->construct_count++;
  }

  auto operator=(const Hashable& rhs) -> Hashable& {
    this->construct_count++;
    id = rhs.id;

    return *this;
  }

  ~Hashable() { destruct_count++; }
  auto hash() const -> Bits_64 { return Func::Hash(id).get_value(); }
  auto operator==(const Hashable& rhs) const -> bool { return rhs.id == id; }

 private:
  Int id;
  Count& construct_count;
  Count& destruct_count;
};

PERIMORTEM_UNIT_TEST(DynamicMap, empty_map) {
  Dynamic::Map<Int, Int> empty_map;
  EXPECT_EQ(empty_map.get_size(), 0);
}

PERIMORTEM_UNIT_TEST(DynamicMap, simple_construction) {
  Dynamic::Map<Int, Int> int_map = {{{1, 2}, {2, 3}, {4, 5}}};

  EXPECT_EQ(int_map.get_size(), 3);
  ASSERT_EQ(int_map[1], 2);
  EXPECT_EQ(int_map[2], 3);
  EXPECT_EQ(int_map[4], 5);
}

PERIMORTEM_UNIT_TEST(DynamicMap, duplicate_keys) {
  Dynamic::Map<Int, Int> int_map = {{{1, 2}, {1, 4}}};

  EXPECT_EQ(int_map.get_size(), 1);
  ASSERT_EQ(int_map[1], 4);

  int_map.insert(2, 8);
  int_map.insert(1, 8);
  ASSERT_EQ(int_map[1], 8);
  EXPECT_EQ(int_map[2], 8);
}

PERIMORTEM_UNIT_TEST(DynamicMap, empty_keys) {
  Dynamic::Map<Dynamic::Bytes, Int> empty_map;

  Int i = 0;
  while (i < 1000) {
    i++;
    empty_map.insert(Dynamic::Bytes(), i);
  }

  // Map should contain a single key which maps to the last value used.
  EXPECT_EQ(empty_map.get_size(), 1);
  EXPECT_EQ(empty_map[Dynamic::Bytes()], i);
}

PERIMORTEM_UNIT_TEST(DynamicMap, insert_stress_test) {
  Dynamic::Map<Int, Int> large_map;

  for (Int i = 0; i < 1000; i++) {
    large_map.insert(i, i + 2);
  }

  EXPECT_EQ(large_map.get_size(), 1000);
  for (Int i = 0; i < 1000; i++) {
    ASSERT_EQ(large_map[i], i + 2);
  }

  EXPECT_EQ(large_map.get_memory_consumption(), 1 << 16);
}

PERIMORTEM_UNIT_TEST(DynamicMap, key_construction_count) {
  Count construct_count = 0;
  Count destruct_count = 0;

  {
    Dynamic::Map<Hashable, Int> custom_map;

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

PERIMORTEM_UNIT_TEST(DynamicMap, value_construction_count) {
  Count construct_count = 0;
  Count destruct_count = 0;

  {
    Dynamic::Map<Int, Hashable> custom_map;

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

PERIMORTEM_UNIT_TEST(DynamicMap, emplace_construction_count) {
  Count construct_count = 0;
  Count destruct_count = 0;

  {
    Dynamic::Map<Int, Hashable> custom_map;

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

PERIMORTEM_UNIT_TEST(DynamicMap, dynamic_keys) {
  Dynamic::Map<Dynamic::Bytes, Int> text_map;

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

PERIMORTEM_UNIT_TEST(DynamicMap, dynamic_value) {
  Dynamic::Map<Int, Dynamic::Bytes> text_map;

  text_map[0] = "Hello"_view;
  text_map[1] = "World"_view;
  text_map[2] = "Longer test string"_view;

  ASSERT_TEXT(text_map[0].get_view(), "Hello"_view);
  ASSERT_TEXT(text_map[1].get_view(), "World"_view);
  ASSERT_TEXT(text_map[2].get_view(), "Longer test string"_view);
}

PERIMORTEM_UNIT_TEST(DynamicMap, leak_test) {
  auto pre_test_memory = Allocator::Bibliotheca::allocated_memory();

  {
    Dynamic::Map<Dynamic::Bytes, Dynamic::Bytes> memory_intensive;
    Dynamic::Bytes source;

    for (Int i = 0; i < 100; i++) {
      source.append('A');
      memory_intensive[source] = "Test text to copy"_view;
    }

    ASSERT_EQ(memory_intensive.get_size(), 100);
  }

  {
    Dynamic::Map<Int, Int> large_map;

    for (Int i = 0; i < 1000; i++) {
      large_map.insert(i, i + 2);
    }
  }

  auto post_test_memory = Allocator::Bibliotheca::allocated_memory();
  EXPECT_EQ(pre_test_memory, post_test_memory);
}