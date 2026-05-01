// Perimortem Engine
// Copyright © Matt Kaes

#include "validation/test/test.hpp"

#include "perimortem/system/uuid.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Perimortem::Utility;
using namespace Perimortem::System;

using namespace Validation;

Test::Harness SystemUuid = {.name = "System::Uuid"};

PERIMORTEM_UNIT_TEST(SystemUuid, default) {
  Uuid uuid;
  constexpr Uuid uuid_const;

  EXPECT(!uuid.is_valid());
  EXPECT_EQ(uuid.get_value()[0], 0);
  EXPECT_EQ(uuid.get_value()[1], 0);

  EXPECT(!uuid_const.is_valid());
  EXPECT_EQ(uuid_const.get_value()[0], 0);
  EXPECT_EQ(uuid_const.get_value()[1], 0);
}

PERIMORTEM_UNIT_TEST(SystemUuid, deserialize_bits_64) {
  const Static::Bytes<36> bytes = {{Bits_64(0x0123456789abcdefULL), Bits_64(0xfedcba9876543210ULL)}};
  Uuid uuid({{Bits_64(0x0123456789abcdefULL), Bits_64(0xfedcba9876543210ULL)}});
  constexpr Uuid uuid_const({{0x0123456789abcdefULL, 0xfedcba9876543210ULL}});

  EXPECT(uuid.is_valid());
  EXPECT_EQ(uuid.get_value()[0], 0x0123456789abcdefULL);
  EXPECT_EQ(uuid.get_value()[1], 0xfedcba9876543210ULL);

  EXPECT(uuid_const.is_valid());
  EXPECT_EQ(uuid_const.get_value()[0], 0x0123456789abcdefULL);
  EXPECT_EQ(uuid_const.get_value()[1], 0xfedcba9876543210ULL);

  EXPECT(uuid == uuid_const);
}

PERIMORTEM_UNIT_TEST(SystemUuid, deserialize_32_nibble) {
  Uuid uuid("0123456789abcdeffedcba9876543210"_bytes);
  constexpr Uuid uuid_const("0123456789abcdeffedcba9876543210"_bytes);

  EXPECT(uuid.is_valid());
  EXPECT_EQ(uuid.get_value()[0], 0x0123456789abcdefULL);
  EXPECT_EQ(uuid.get_value()[1], 0xfedcba9876543210ULL);

  EXPECT(uuid_const.is_valid());
  EXPECT_EQ(uuid_const.get_value()[0], 0x0123456789abcdefULL);
  EXPECT_EQ(uuid_const.get_value()[1], 0xfedcba9876543210ULL);

  EXPECT(uuid == uuid_const);
}