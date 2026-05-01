// Perimortem Engine
// Copyright © Matt Kaes

#include "validation/test/test.hpp"

#include "perimortem/system/uuid.hpp"
#include "perimortem/serialization/encoding/null_terminated.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Perimortem::System;

using namespace Validation;

Test::Harness SystemUuid = {.name = "System::Uuid"};

PERIMORTEM_UNIT_TEST(SystemUuid, default) {
  Uuid uuid;
  constexpr Uuid uuid_const;

  EXPECT(!uuid.is_set());
  EXPECT_EQ(uuid.get_value()[0], 0);
  EXPECT_EQ(uuid.get_value()[1], 0);

  EXPECT(!uuid_const.is_set());
  EXPECT_EQ(uuid_const.get_value()[0], 0);
  EXPECT_EQ(uuid_const.get_value()[1], 0);
}

PERIMORTEM_UNIT_TEST(SystemUuid, deserialize_bits_64) {
  Uuid uuid{0x0123456789abcdefULL, 0xfedcba9876543210ULL};
  constexpr Uuid uuid_const{0x0123456789abcdefULL, 0xfedcba9876543210ULL};

  EXPECT(uuid.is_set());
  EXPECT_EQ(uuid.get_value()[0], 0x0123456789abcdefULL);
  EXPECT_EQ(uuid.get_value()[1], 0xfedcba9876543210ULL);

  EXPECT(uuid_const.is_set());
  EXPECT_EQ(uuid_const.get_value()[0], 0x0123456789abcdefULL);
  EXPECT_EQ(uuid_const.get_value()[1], 0xfedcba9876543210ULL);

  EXPECT(uuid == uuid_const);
}

PERIMORTEM_UNIT_TEST(SystemUuid, deserialize_32_nibble) {
  Uuid uuid("0123456789abcdeffedcba9876543210"_bytes);
  constexpr Uuid uuid_const("0123456789abcdeffedcba9876543210"_bytes);

  EXPECT(uuid.is_set());
  EXPECT_EQ(uuid.get_value()[0], 0x0123456789abcdefULL);
  EXPECT_EQ(uuid.get_value()[1], 0xfedcba9876543210ULL);

  EXPECT(uuid_const.is_set());
  EXPECT_EQ(uuid_const.get_value()[0], 0x0123456789abcdefULL);
  EXPECT_EQ(uuid_const.get_value()[1], 0xfedcba9876543210ULL);

  EXPECT(uuid == uuid_const);
}

PERIMORTEM_UNIT_TEST(SystemUuid, deserialize_uuid_v4) {
  Uuid uuid("cb8c03d8-2b01-4a39-b31d-2c6805e6503c"_bytes);
  constexpr Uuid uuid_const("cb8c03d8-2b01-4a39-b31d-2c6805e6503c"_bytes);

  EXPECT(uuid.is_set());
  EXPECT_EQ(uuid.get_value()[0], 0xcb8c03d82b014a39ULL);
  EXPECT_EQ(uuid.get_value()[1], 0xb31d2c6805e6503cULL);

  EXPECT(uuid_const.is_set());
  EXPECT_EQ(uuid_const.get_value()[0], 0xcb8c03d82b014a39ULL);
  EXPECT_EQ(uuid_const.get_value()[1], 0xb31d2c6805e6503cULL);

  EXPECT(uuid == uuid_const);
}

PERIMORTEM_UNIT_TEST(SystemUuid, generate) {
  Uuid uuid1 = Uuid::generate();
  Uuid uuid2 = Uuid::generate();

  EXPECT(uuid1.is_set());
  EXPECT(uuid2.is_set());
  EXPECT(uuid1 != uuid2);
}