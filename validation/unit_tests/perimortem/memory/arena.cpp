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

class FactoryValue {
 public:
  FactoryValue(const FactoryValue&) = delete;
  FactoryValue(FactoryValue&&) = delete;

  static auto create(Allocator::Arena& arena, Unsigned_32 value, Bool selected)
      -> FactoryValue& {
    return arena.construct_from<FactoryValue>(
        [=]() { return FactoryValue(value, selected); });
  }

  constexpr auto get_value() const -> Unsigned_32 { return value; }
  constexpr auto is_selected() const -> Bool { return selected; }

 private:
  constexpr FactoryValue(Unsigned_32 value, Bool selected)
      : value(value), selected(selected) {}

  Unsigned_32 value;
  Bool selected;
};

static_assert(!__is_constructible(FactoryValue, Unsigned_32, Bool));
static_assert(!__is_constructible(FactoryValue, const FactoryValue&));
static_assert(!__is_constructible(FactoryValue, FactoryValue&&));

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

PERIMORTEM_UNIT_TEST(Arena, owner_factory) {
  Allocator::Arena arena;

  FactoryValue& value = FactoryValue::create(arena, 42, True);

  EXPECT_EQ(value.get_value(), Unsigned_32(42));
  EXPECT(value.is_selected());
}
