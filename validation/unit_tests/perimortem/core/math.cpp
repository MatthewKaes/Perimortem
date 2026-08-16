// Perimortem Engine
// Copyright © Matt Kaes

#include "perimortem/core/math.hpp"

#include "validation/unit_test.hpp"

using namespace Perimortem::Core;
using namespace Validation;

static Harness CoreMath = {
  .name = "Perimortem::Core::Math"_view,
};

PERIMORTEM_UNIT_TEST(CoreMath, integer_width_fit) {
  EXPECT(Math::is_representable(Signed_64(-128), 1));
  EXPECT(Math::is_representable(Signed_64(127), 1));
  EXPECT_NOT(Math::is_representable(Signed_64(-129), 1));
  EXPECT_NOT(Math::is_representable(Signed_64(128), 1));
  EXPECT(Math::is_representable(Signed_64(-9223372036854775807LL - 1), 8));
  EXPECT(Math::is_representable(Signed_64(9223372036854775807LL), 8));

  EXPECT(Math::is_representable(Unsigned_64(0), 1));
  EXPECT(Math::is_representable(Unsigned_64(255), 1));
  EXPECT_NOT(Math::is_representable(Unsigned_64(256), 1));
  EXPECT(Math::is_representable(Unsigned_64(-1), 8));

  EXPECT_NOT(Math::is_representable(Signed_64(0), 0));
  EXPECT_NOT(Math::is_representable(Signed_64(0), 9));
  EXPECT_NOT(Math::is_representable(Unsigned_64(0), 0));
  EXPECT_NOT(Math::is_representable(Unsigned_64(0), 9));
}
