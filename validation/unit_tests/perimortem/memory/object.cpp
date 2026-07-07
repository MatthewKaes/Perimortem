// Perimortem Engine
// Copyright © Matt Kaes

#include "perimortem/memory/dynamic/object.hpp"

#include "validation/unit_test.hpp"

#include "perimortem/core/null_terminated.hpp"

#include "perimortem/memory/dynamic/map.hpp"

using namespace Perimortem::Memory;
using namespace Validation;

static Harness DynamicObject = {
  .name = "Dynamic::Object"_view,
};

class RaiiProbe {
 public:
  RaiiProbe(Count& destructor_count, Count value = 0)
      : destructor_count(destructor_count), value(value) {}

  ~RaiiProbe() { destructor_count++; }

  constexpr auto get_value() const -> Count { return value; }

 private:
  Count& destructor_count;
  Count value = 0;
};

PERIMORTEM_UNIT_TEST(DynamicObject, shared_lifetime) {
  Count destructor_count = 0;

  {
    Dynamic::Object<RaiiProbe> probe(destructor_count, 42);
    EXPECT_EQ(probe->get_value(), Count(42));

    {
      Dynamic::Object<RaiiProbe> second = probe;
      EXPECT_EQ(second->get_value(), Count(42));
      EXPECT_EQ(destructor_count, Count(0));
    }

    EXPECT_EQ(destructor_count, Count(0));
  }

  EXPECT_EQ(destructor_count, Count(1));
}

PERIMORTEM_UNIT_TEST(DynamicObject, assignment) {
  Count destructor_count = 0;

  {
    Dynamic::Object<RaiiProbe> first(destructor_count, 1);
    Dynamic::Object<RaiiProbe> second(destructor_count, 2);

    second = first;
    EXPECT_EQ(destructor_count, Count(1));
    EXPECT_EQ(second->get_value(), Count(1));
  }

  EXPECT_EQ(destructor_count, Count(2));
}

PERIMORTEM_UNIT_TEST(DynamicObject, move_assignment) {
  Count destructor_count = 0;

  {
    Dynamic::Object<RaiiProbe> first(destructor_count, 1);
    {
      Dynamic::Object<RaiiProbe> second(destructor_count, 2);

      first = static_cast<Dynamic::Object<RaiiProbe>&&>(second);
      EXPECT_EQ(first->get_value(), Count(2));
      EXPECT_EQ(second->get_value(), Count(1));
      EXPECT_EQ(destructor_count, Count(0));
    }

    EXPECT_EQ(first->get_value(), Count(2));
    EXPECT_EQ(destructor_count, Count(1));
  }

  EXPECT_EQ(destructor_count, Count(2));
}

PERIMORTEM_UNIT_TEST(DynamicObject, move_construction) {
  Count destructor_count = 0;

  {
    Dynamic::Object<RaiiProbe> first(destructor_count, 3);
    {
      Dynamic::Object<RaiiProbe> second(
          static_cast<Dynamic::Object<RaiiProbe>&&>(first));
      EXPECT_EQ(first->get_value(), Count(3));
      EXPECT_EQ(second->get_value(), Count(3));
    }

    EXPECT_EQ(destructor_count, Count(0));
  }

  EXPECT_EQ(destructor_count, Count(1));
}

PERIMORTEM_UNIT_TEST(DynamicObject, map_owner) {
  Count destructor_count = 0;
  Dynamic::Map<Count, Dynamic::Object<RaiiProbe>> values;

  {
    Dynamic::Object<RaiiProbe> probe(destructor_count, 7);
    values.insert(0, probe);
  }

  EXPECT_EQ(destructor_count, Count(0));
  EXPECT_EQ(values.find(0)->value->get_value(), Count(7));

  values.remove(0);
  EXPECT_EQ(destructor_count, Count(1));
}

PERIMORTEM_UNIT_TEST(DynamicObject, map_rehash) {
  Count destructor_count = 0;
  Dynamic::Map<Count, Dynamic::Object<RaiiProbe>> values;

  for (Count i = 0; i < 16; i++) {
    Dynamic::Object<RaiiProbe> probe(destructor_count, i);
    values.insert(i, probe);
  }

  EXPECT_EQ(destructor_count, Count(0));
  for (Count i = 0; i < 16; i++) {
    EXPECT_EQ(values.find(i)->value->get_value(), i);
  }

  values.clear();
  EXPECT_EQ(destructor_count, Count(16));
}
