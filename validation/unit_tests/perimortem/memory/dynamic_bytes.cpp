// Perimortem Engine
// Copyright © Matt Kaes

#include "validation/unit_test.hpp"

#include "perimortem/memory/dynamic/bytes.hpp"

#include "perimortem/abi/memory/dynamic/bytes.hpp"

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

PERIMORTEM_UNIT_TEST(DynamicBytes, copies_share_until_written) {
  Dynamic::Bytes original("shared"_view);
  Dynamic::Bytes copied(original);

  const Unsigned_8* shared = original.get_view().get_data();
  EXPECT(copied.get_view().get_data() == shared);

  copied.append(Unsigned_8('!'));

  EXPECT_EQ(original.get_view(), "shared"_view);
  EXPECT_EQ(copied.get_view(), "shared!"_view);
  EXPECT(copied.get_view().get_data() != shared);
}

PERIMORTEM_UNIT_TEST(DynamicBytes, writable_access_detaches) {
  Dynamic::Bytes original("shared"_view);
  Dynamic::Bytes copied(original);

  Access::Bytes access = copied.get_access();
  auto first = access[0];
  EXPECT(first);
  *first = Unsigned_8('S');

  EXPECT_EQ(original.get_view(), "shared"_view);
  EXPECT_EQ(copied.get_view(), "Shared"_view);
  EXPECT(copied.get_view().get_data() != original.get_view().get_data());
}

PERIMORTEM_UNIT_TEST(DynamicBytes, slices_remain_borrowed) {
  Dynamic::Bytes bytes("borrowed"_view);
  const Unsigned_8* allocation = bytes.get_view().get_data();

  View::Bytes slice = bytes.slice(2, 4);

  EXPECT_EQ(slice, "rrow"_view);
  EXPECT(slice.get_data() == allocation + 2);
}

PERIMORTEM_UNIT_TEST(DynamicBytes, reset_releases_once) {
  Dynamic::Bytes bytes("released"_view);

  bytes.reset();
  bytes.reset();

  EXPECT(bytes.is_empty());
  EXPECT_EQ(bytes.get_capacity(), Count(0));
  EXPECT(!bytes.get_view().get_data());
}

PERIMORTEM_UNIT_TEST(DynamicBytes, abi_carrier_is_three_words) {
  EXPECT_EQ(
      sizeof(Perimortem::Abi::Memory::Dynamic::Bytes),
      sizeof(Unsigned_8*) + sizeof(Count) + sizeof(Count));
  EXPECT_EQ(
      sizeof(Dynamic::Bytes), sizeof(Perimortem::Abi::Memory::Dynamic::Bytes));
}
