// Perimortem Engine
// Copyright © Matt Kaes

#include "perimortem/utility/option.hpp"

#include "validation/unit_test.hpp"

#include "perimortem/core/null_terminated.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Utility;
using namespace Validation;

static Harness UtilityOption = {
  .name = "Utility::Option"_view,
};

class BorrowedBase {};
class BorrowedDerived final : public BorrowedBase {};

PERIMORTEM_UNIT_TEST(UtilityOption, visits_none) {
  Option<const Signed_32&> selected;

  Count branch = selected.visit(
      [](const None&) { return Count(1); },
      [](const Signed_32&) { return Count(2); });

  EXPECT_EQ(branch, 1);
}

PERIMORTEM_UNIT_TEST(UtilityOption, visits_reference) {
  Signed_32 value = 41;
  Option<Signed_32&> selected(value);

  selected.visit([](const None&) {}, [](Signed_32& found) -> void { found++; });

  EXPECT_EQ(value, 42);
}

PERIMORTEM_UNIT_TEST(UtilityOption, copies_borrow) {
  const Signed_32 value = 42;
  Option<const Signed_32&> first(value);
  Option<const Signed_32&> second(first);

  Signed_32 found = second.visit(
      [](const None&) { return Signed_32(0); },
      [](const Signed_32& selected) { return selected; });

  EXPECT_EQ(found, value);
}

static_assert(sizeof(Option<const Signed_32&>) == sizeof(const Signed_32*));
static_assert(__is_trivially_copyable(Option<const Signed_32&>));
static_assert(__is_constructible(Option<const Signed_32&>, const Signed_32&));
static_assert(!__is_constructible(Option<const Signed_32&>, Signed_32&&));
static_assert(
    !__is_constructible(Option<const BorrowedBase&>, BorrowedDerived&&));
