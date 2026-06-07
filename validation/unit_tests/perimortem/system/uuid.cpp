// Perimortem Engine
// Copyright © Matt Kaes

#include "perimortem/system/uuid.hpp"

#include "validation/unit_test.hpp"

#include "perimortem/core/null_terminated.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::System;

using namespace Validation;

static Harness SystemUuid = {
  .name = "System::Uuid"_view,
};

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

PERIMORTEM_UNIT_TEST(SystemUuid, serialize_uuid_v4) {
  Uuid uuid(0xcb8c03d82b014a39ULL, 0xb31d2c6805e6503cULL);

  EXPECT_TEXT(uuid.serialize(), "cb8c03d8-2b01-4a39-b31d-2c6805e6503c"_bytes);
}

PERIMORTEM_UNIT_TEST(SystemUuid, roundtrip_uuid) {
  auto source = "cb8c03d8-2b01-4a39-b31d-2c6805e6503c"_bytes;
  Uuid uuid(source);

  EXPECT_TEXT(uuid.serialize(), source);
}

PERIMORTEM_UNIT_TEST(SystemUuid, generate_v4) {
  Uuid uuid1 = Uuid::generate_v4();
  Uuid uuid2 = Uuid::generate_v4();

  EXPECT(uuid1.is_set());
  EXPECT(uuid2.is_set());
  EXPECT(uuid1 != uuid2);
}

PERIMORTEM_UNIT_TEST(SystemUuid, generate_v7) {
  Uuid uuid1 = Uuid::generate_v7();
  Uuid uuid2 = Uuid::generate_v7();

  auto output = uuid1.serialize();
  EXPECT(output.hash());
  EXPECT(uuid1.is_set());
  EXPECT(uuid2.is_set());
  EXPECT(uuid1 != uuid2);
}
