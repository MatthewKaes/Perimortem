// Perimortem Engine
// Copyright © Matt Kaes

#include "perimortem/system/version.hpp"

#include "validation/unit_test.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::System;
using namespace Validation;

static Harness SystemVersion = {
  .name = "System::Version"_view,
};

PERIMORTEM_UNIT_TEST(SystemVersion, value) {
  constexpr Version unset;
  constexpr Version development(0, 1);
  constexpr Version release(1, 0);
  constexpr Version maximum(Unsigned_16(-1), Unsigned_16(-1));

  EXPECT(unset.is_null());
  EXPECT_EQ(unset.get_major(), Unsigned_16(0));
  EXPECT_EQ(unset.get_minor(), Unsigned_16(0));

  EXPECT_NOT(development.is_null());
  EXPECT_EQ(development.get_major(), Unsigned_16(0));
  EXPECT_EQ(development.get_minor(), Unsigned_16(1));
  EXPECT(development != release);
  EXPECT(development < release);
  EXPECT(release > development);
  EXPECT(Version(1, 1) > release);
  EXPECT_NOT(Version(1, 1) < release);

  EXPECT_EQ(maximum.get_major(), Unsigned_16(-1));
  EXPECT_EQ(maximum.get_minor(), Unsigned_16(-1));
  EXPECT_EQ(sizeof(Version), Count(4));
}

PERIMORTEM_UNIT_TEST(SystemVersion, canonical_text) {
  constexpr Version development = Version::parse("0.1"_view);
  constexpr Version release = Version::parse("12.34"_view);
  constexpr Version maximum = Version::parse("65535.65535"_view);

  EXPECT(development == Version(0, 1));
  EXPECT(release == Version(12, 34));
  EXPECT(maximum == Version(Unsigned_16(-1), Unsigned_16(-1)));

  EXPECT(Version::parse("0.0"_view).is_null());
  EXPECT(Version::parse("1"_view).is_null());
  EXPECT(Version::parse("1."_view).is_null());
  EXPECT(Version::parse(".1"_view).is_null());
  EXPECT(Version::parse("01.2"_view).is_null());
  EXPECT(Version::parse("1.02"_view).is_null());
  EXPECT(Version::parse("1.2.3"_view).is_null());
  EXPECT(Version::parse("65536.0"_view).is_null());
  EXPECT(Version::parse("0.65536"_view).is_null());
}
