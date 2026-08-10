// Perimortem Engine
// Copyright © Matt Kaes

#include "validation/unit_test.hpp"

#include "perimortem/core/perimortem.hpp"

using namespace Validation;

static Harness CoreControl = {
  .name = "Perimortem::Core::Control"_view,
};

static auto select_result(Bool reject, const Signed_32& value)
    -> const Signed_32* {
  BAIL_IF(reject);
  return &value;
}

static auto select_nested_result(
    Bool outer,
    Bool reject,
    const Signed_32& selected,
    const Signed_32& fallback) -> const Signed_32* {
  // Exercise the macro as the only statement in an outer branch so it retains
  // statement semantics when nested beside an else.
  if (outer) {
    BAIL_IF(reject);
  } else {
    return &fallback;
  }

  return &selected;
}

static auto observe_condition(Count& observations, Bool reject) -> Bool {
  BAIL_IF((observations++, reject));
  return True;
}

PERIMORTEM_UNIT_TEST(CoreControl, conditional_return) {
  Signed_32 value = 42;
  EXPECT(select_result(False, value) == &value);
  EXPECT(select_result(True, value) == nullptr);
}

PERIMORTEM_UNIT_TEST(CoreControl, complete_statement) {
  Signed_32 selected = 42;
  Signed_32 fallback = 40;
  EXPECT(select_nested_result(False, False, selected, fallback) == &fallback);
  EXPECT(select_nested_result(True, False, selected, fallback) == &selected);
  EXPECT(select_nested_result(True, True, selected, fallback) == nullptr);
}

PERIMORTEM_UNIT_TEST(CoreControl, observes_once) {
  Count observations = 0;
  EXPECT(observe_condition(observations, False));
  EXPECT_EQ(observations, Count(1));
  EXPECT_NOT(observe_condition(observations, True));
  EXPECT_EQ(observations, Count(2));
}
