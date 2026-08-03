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

class StackValue {
 public:
  StackValue(Signed_32 value, Count& destructions)
      : value(value), destructions(destructions) {}

  StackValue(const StackValue& source)
      : value(source.value), destructions(source.destructions) {}
  StackValue(StackValue&& source)
      : value(source.value), destructions(source.destructions) {
    source.moved = True;
  }

  ~StackValue() {
    if (!moved) {
      destructions++;
    }
  }

  auto increment() -> void { value++; }
  auto get() const -> Signed_32 { return value; }

 private:
  Signed_32 value;
  Bool moved = False;
  Count& destructions;
};

static auto create_stack_value(Count& destructions) -> Option<StackValue> {
  StackValue value(41, destructions);
  return Data::take(value);
}

PERIMORTEM_UNIT_TEST(UtilityOption, visits_none) {
  Option<const Signed_32&> selected;

  Count branch = selected.visit(
      []() { return Count(1); }, [](const Signed_32&) { return Count(2); });

  EXPECT_EQ(branch, 1);
}

PERIMORTEM_UNIT_TEST(UtilityOption, visits_reference) {
  Signed_32 value = 41;
  Option<Signed_32&> selected(value);

  selected.visit([]() {}, [](Signed_32& found) -> void { found++; });

  EXPECT_EQ(value, 42);
}

PERIMORTEM_UNIT_TEST(UtilityOption, copies_borrow) {
  const Signed_32 value = 42;
  Option<const Signed_32&> first(value);
  Option<const Signed_32&> second(first);

  Signed_32 found = second.visit(
      []() { return Signed_32(0); },
      [](const Signed_32& selected) { return selected; });

  EXPECT_EQ(found, value);
}

PERIMORTEM_UNIT_TEST(UtilityOption, copies_value) {
  Option<Signed_32> first(41);
  Option<Signed_32> second(first);

  second.visit([]() {}, [](Signed_32& selected) -> void { selected++; });

  Signed_32 first_value = first.visit(
      []() { return Signed_32(0); },
      [](Signed_32 selected) { return selected; });
  Signed_32 second_value = second.visit(
      []() { return Signed_32(0); },
      [](Signed_32 selected) { return selected; });

  EXPECT_EQ(first_value, 41);
  EXPECT_EQ(second_value, 42);
}

PERIMORTEM_UNIT_TEST(UtilityOption, owns_stack_value) {
  Count destructions = 0;

  {
    Option<StackValue> selected = create_stack_value(destructions);
    Option<StackValue> moved(Data::take(selected));

    moved.visit([]() {}, [](StackValue& value) -> void { value.increment(); });

    const Option<StackValue>& observed = moved;
    Signed_32 found = observed.visit(
        []() { return Signed_32(0); },
        [](const StackValue& value) { return value.get(); });

    EXPECT_EQ(found, 42);
  }

  EXPECT_EQ(destructions, Count(1));
}

PERIMORTEM_UNIT_TEST(UtilityOption, arrow_access) {
  Count owned_destructions = 0;
  Option<StackValue> owned = create_stack_value(owned_destructions);
  owned->increment();

  const Option<StackValue>& const_owned = owned;
  EXPECT_EQ(const_owned->get(), 42);

  Count borrowed_destructions = 0;
  StackValue value(40, borrowed_destructions);
  Option<StackValue&> borrowed(value);
  borrowed->increment();

  const Option<StackValue&>& const_borrowed = borrowed;
  const_borrowed->increment();
  EXPECT_EQ(value.get(), 42);
}

PERIMORTEM_UNIT_TEST(UtilityOption, accepts_empty) {
  Count destructions = 0;
  Option<StackValue> selected = create_stack_value(destructions);
  selected = {};

  Count branch = selected.visit(
      []() { return Count(1); }, [](const StackValue&) { return Count(2); });

  EXPECT_EQ(branch, Count(1));
  EXPECT_EQ(destructions, Count(1));
}

static_assert(sizeof(Option<const Signed_32&>) == sizeof(const Signed_32*));
static_assert(__is_trivially_copyable(Option<const Signed_32&>));
static_assert(__is_constructible(Option<const Signed_32&>, const Signed_32&));
static_assert(!__is_constructible(Option<const Signed_32&>, Signed_32&&));
static_assert(
    !__is_constructible(Option<const BorrowedBase&>, BorrowedDerived&&));
static_assert(__is_constructible(Option<StackValue>, StackValue&&));
static_assert(__is_constructible(Option<StackValue>, Option<StackValue>&&));
