// Perimortem Engine
// Copyright © Matt Kaes

#include "perimortem/memory/allocator/arena.hpp"

#include "validation/unit_test.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Validation;

static Harness Arena = {
  .name = "Memory::Allocator::Arena"_view,
};

PERIMORTEM_UNIT_TEST(Arena, reserve) {
  class RequiredValue {
   public:
    constexpr RequiredValue(Unsigned_32 value) : value(value) {}

    constexpr auto get_value() const -> Unsigned_32 { return value; }

   private:
    Unsigned_32 value;
  };

  Allocator::Arena arena;
  Unsigned_32& scalar = arena.reserve<Unsigned_32>();
  auto empty = arena.reserve<RequiredValue>(0);
  auto values = arena.reserve<RequiredValue>(2);

  scalar = 7;
  values[0] = RequiredValue(1);
  values[1] = RequiredValue(2);

  EXPECT_EQ(scalar, Unsigned_32(7));
  EXPECT(empty.get_data() != nullptr);
  EXPECT_EQ(empty.get_size(), Count(0));
  EXPECT_EQ(values.get_size(), Count(2));
  EXPECT_EQ(values[0].get_value(), Unsigned_32(1));
  EXPECT_EQ(values[1].get_value(), Unsigned_32(2));
}
