// Perimortem Engine
// Copyright © Matt Kaes

#include "perimortem/utility/result.hpp"

#include "validation/unit_test.hpp"

#include "perimortem/core/null_terminated.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Utility;
using namespace Validation;

static Harness UtilityResult = {
  .name = "Utility::Result"_view,
};

enum class ResultError : Unsigned_8 {
  Unknown = Unsigned_8(-1),
  Rejected = 0,
};

PERIMORTEM_UNIT_TEST(UtilityResult, visits_value) {
  Result<Signed_32, ResultError> selected(Signed_32(41));

  selected.visit([](Signed_32& value) -> void { value++; }, [](ResultError) {});

  const auto& observed = selected;
  Signed_32 value = observed.visit(
      [](Signed_32 selected) { return selected; },
      [](ResultError) { return Signed_32(0); });
  EXPECT_EQ(value, Signed_32(42));
}

PERIMORTEM_UNIT_TEST(UtilityResult, visits_error) {
  Result<Signed_32, ResultError> selected(ResultError::Rejected);
  Result<Signed_32, ResultError> copied(selected);
  Result<Signed_32, ResultError> moved(Data::take(copied));

  ResultError error = moved.visit(
      [](Signed_32) { return ResultError::Unknown; },
      [](ResultError selected) { return selected; });

  EXPECT(error == ResultError::Rejected);
}

PERIMORTEM_UNIT_TEST(UtilityResult, preserves_reference) {
  Signed_32 value = 41;
  Result<Signed_32&, ResultError> selected(value);

  selected.visit(
      [](Signed_32& selected) -> void { selected++; }, [](ResultError) {});

  EXPECT_EQ(value, Signed_32(42));
}

static_assert(!__is_constructible(Result<Signed_32, ResultError>));
static_assert(__is_constructible(Result<Signed_32, ResultError>, Signed_32));
static_assert(__is_constructible(Result<Signed_32, ResultError>, ResultError));
static_assert(
    !__is_constructible(Result<Signed_32&, ResultError>, Signed_32&&));
