// Perimortem Engine
// Copyright © Matt Kaes

#include "perimortem/core/static/union.hpp"

#include "validation/unit_test.hpp"

#include "perimortem/core/null_terminated.hpp"

using namespace Perimortem::Core;
using namespace Validation;

static Harness CoreUnion = {
  .name = "Core::Static::Union"_view,
};

PERIMORTEM_UNIT_TEST(CoreUnion, null) {
  Static::Union<Unsigned_32, View::Bytes, Bool> value;

  EXPECT(value.is_null());
  EXPECT(value.find<Unsigned_32>() == nullptr);
  EXPECT(value.find<View::Bytes>() == nullptr);
}

PERIMORTEM_UNIT_TEST(CoreUnion, alternatives) {
  Static::Union<Unsigned_32, View::Bytes, Bool> bits(Unsigned_32(42));
  Static::Union<Unsigned_32, View::Bytes, Bool> text("open"_view);
  Static::Union<Unsigned_32, View::Bytes, Bool> flag(True);

  EXPECT_EQ(*bits.find<Unsigned_32>(), Unsigned_32(42));
  EXPECT_TEXT(*text.find<View::Bytes>(), "open"_view);
  EXPECT(*flag.find<Bool>());
}

PERIMORTEM_UNIT_TEST(CoreUnion, visit) {
  auto read = [](const auto& source) {
    return source.visit(
        []() { return Count(1); }, [](Unsigned_32 bits) { return Count(bits); },
        [](View::Bytes text) { return text.get_size(); });
  };

  Static::Union<Unsigned_32, View::Bytes> empty;
  Static::Union<Unsigned_32, View::Bytes> bits(Unsigned_32(42));
  Static::Union<Unsigned_32, View::Bytes> text("visit"_view);
  EXPECT_EQ(read(empty), Count(1));
  EXPECT_EQ(read(bits), Count(42));
  EXPECT_EQ(read(text), Count(5));
}

PERIMORTEM_UNIT_TEST(CoreUnion, copy_move) {
  Static::Union<Unsigned_32, View::Bytes> first("copy"_view);
  Static::Union<Unsigned_32, View::Bytes> second(first);
  Static::Union<Unsigned_32, View::Bytes> third(Data::take(second));

  EXPECT_TEXT(*third.find<View::Bytes>(), "copy"_view);
}

PERIMORTEM_UNIT_TEST(CoreUnion, equality) {
  Static::Union<Unsigned_64> hundred(Unsigned_64(100));
  Static::Union<Unsigned_64> another_hundred(Unsigned_64(100));
  Static::Union<Unsigned_64> different(Unsigned_64(101));
  Static::Union<Unsigned_64> empty;
  Static::Union<Unsigned_64, View::Bytes> bits(Unsigned_64(100));
  Static::Union<Unsigned_64, View::Bytes> text("100"_view);

  EXPECT(hundred == 100);
  EXPECT(hundred == another_hundred);
  EXPECT(hundred != different);
  EXPECT(different != 100);
  EXPECT(empty == Static::Union<Unsigned_64>());
  EXPECT(empty != hundred);
  EXPECT(bits != text);
}

static_assert(sizeof(Static::Union<Unsigned_32, Unsigned_64>) <= 16);
static_assert(sizeof(Static::Union<View::Bytes, Signed_64>) == 24);
static_assert(__is_constructible(Static::Union<Unsigned_64>, int));
static_assert(!__is_constructible(Static::Union<Unsigned_64, Signed_64>, int));
static_assert(
    __is_trivially_destructible(Static::Union<View::Bytes, Signed_64>));
