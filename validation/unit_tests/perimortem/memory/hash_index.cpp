// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "perimortem/memory/hash_index.h"

#include "validation/unit_test.hpp"

#include "perimortem/core/null_terminated.hpp"
#include "perimortem/core/hash.h"

#include "perimortem/memory/aligned_buffer.h"

using namespace Perimortem::Core;
using namespace Validation;

static Harness HashIndex = {
  .name = "Memory::HashIndex"_view,
};

PERIMORTEM_UNIT_TEST(HashIndex, aligned_storage) {
  perimortem_aligned_buffer buffer = {};
  ASSERT(perimortem_aligned_buffer_create(17, 16, &buffer));
  EXPECT(buffer.data != nullptr);
  EXPECT(buffer.capacity >= Count(17));
  EXPECT_EQ(reinterpret_cast<Count>(buffer.data) & 15, Count(0));
  perimortem_aligned_buffer_release(&buffer);
  EXPECT(buffer.data == nullptr);
  EXPECT_EQ(buffer.capacity, Count(0));
}

PERIMORTEM_UNIT_TEST(HashIndex, stable_hash_contract) {
  static constexpr U8 sample[] = "Tetrodotoxin";
  EXPECT_EQ(perimortem_hash_u64(1), UINT64_C(0xA833BFC15CFBD657));
  EXPECT_EQ(
      perimortem_hash_bytes({nullptr, 0}), UINT64_C(0x1FC221792E40C189));
  EXPECT_EQ(
      perimortem_hash_bytes({sample, 12}), UINT64_C(0x2AA166F63B56B39A));
  EXPECT_EQ(
      perimortem_hash_combine(perimortem_hash_u64(7), 11),
      UINT64_C(0x7C76A7B6FDA1C782));
}

PERIMORTEM_UNIT_TEST(HashIndex, collision_candidates) {
  perimortem_hash_index index = {};
  ASSERT(perimortem_hash_index_insert(&index, 7, 2));
  ASSERT(perimortem_hash_index_insert(&index, 7, 4));
  ASSERT(perimortem_hash_index_insert(&index, 15, 6));

  perimortem_hash_index_match match = {};
  ASSERT(perimortem_hash_index_find_first(&index, 7, &match));
  Count first = match.entry;
  ASSERT(perimortem_hash_index_find_next(&index, 7, &match));
  Count second = match.entry;
  EXPECT((first == 2 && second == 4) || (first == 4 && second == 2));
  EXPECT_NOT(perimortem_hash_index_find_next(&index, 7, &match));
  perimortem_hash_index_release(&index);
}

PERIMORTEM_UNIT_TEST(HashIndex, removal_repairs_cluster) {
  perimortem_hash_index index = {};
  ASSERT(perimortem_hash_index_insert(&index, 1, 10));
  ASSERT(perimortem_hash_index_insert(&index, 3, 20));
  ASSERT(perimortem_hash_index_insert(&index, 5, 30));
  ASSERT(perimortem_hash_index_remove(&index, 1, 10));

  perimortem_hash_index_match match = {};
  ASSERT(perimortem_hash_index_find_first(&index, 3, &match));
  EXPECT_EQ(match.entry, Count(20));
  ASSERT(perimortem_hash_index_find_first(&index, 5, &match));
  EXPECT_EQ(match.entry, Count(30));
  EXPECT_NOT(perimortem_hash_index_find_first(&index, 1, &match));
  perimortem_hash_index_release(&index);
}

PERIMORTEM_UNIT_TEST(HashIndex, moved_entry_index) {
  perimortem_hash_index index = {};
  ASSERT(perimortem_hash_index_insert(&index, 42, 8));
  ASSERT(perimortem_hash_index_replace(&index, 42, 8, 3));

  perimortem_hash_index_match match = {};
  ASSERT(perimortem_hash_index_find_first(&index, 42, &match));
  EXPECT_EQ(match.entry, Count(3));
  perimortem_hash_index_clear(&index);
  EXPECT_NOT(perimortem_hash_index_find_first(&index, 42, &match));
  perimortem_hash_index_release(&index);
}
