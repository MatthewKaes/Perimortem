// Perimortem Engine
// Copyright © Matt Kaes

#include "perimortem/core/view/vector.hpp"

#include "validation/unit_test.hpp"

using namespace Perimortem::Core;
using namespace Validation;

static Harness CoreViewVector = {
  .name = "Core::View::Vector"_view,
};

PERIMORTEM_UNIT_TEST(CoreViewVector, contains) {
  struct NamedValue {
    View::Bytes name;
    Unsigned_32 value;
  };

  View::Bytes text[] = {"First"_view, "Second"_view};
  View::Vector<View::Bytes> text_values(text);

  EXPECT(text_values.contains("Second"_view));
  EXPECT(!text_values.contains("Missing"_view));

  Unsigned_64 numbers[] = {1, 2};
  View::Vector<Unsigned_64> numeric_values(numbers);

  EXPECT(numeric_values.contains(Unsigned_32(2)));

  NamedValue named[] = {
    {.name = "First"_view, .value = 1},
    {.name = "Second"_view, .value = 2},
    {.name = "Third"_view, .value = 3},
  };
  View::Vector<NamedValue> named_values(named);
  Count visited = 0;

  Bool found = named_values.contains([&visited](const NamedValue& candidate) {
    visited++;
    return candidate.name == "Second"_view;
  });

  EXPECT(found);
  EXPECT_EQ(visited, Count(2));
  EXPECT(!named_values.contains(
      [](const NamedValue& candidate) { return candidate.value == 4; }));
}
