// Perimortem Engine
// Copyright © Matt Kaes

#include "validation/unit_test.hpp"

#include "perimortem/core/view/bytes.hpp"

#include "Validation.TtxTests/cpp_abi.hpp"

using namespace Perimortem::Core;
using namespace Validation;

static Harness TtxAbi = {
  .name = "TTX::ABI"_view,
};

static View::Bytes printed[4];
static Count printed_count = 0;

extern "C" void print(View::Bytes data) {
  if (printed_count < 4) {
    printed[printed_count++] = data;
  }
}

extern "C" Count ttx_test_add(Count left, Count right) {
  return left + right;
}

extern "C" Count ttx_test_sum7(
    Count a,
    Count b,
    Count c,
    Count d,
    Count e,
    Count f,
    Count g) {
  return a + b + c + d + e + f + g;
}

extern "C" Real_64 ttx_test_real9(
    Real_64 a,
    Real_64 b,
    Real_64 c,
    Real_64 d,
    Real_64 e,
    Real_64 f,
    Real_64 g,
    Real_64 h,
    Real_64 i) {
  return a + b + c + d + e + f + g + h + i;
}

PERIMORTEM_UNIT_TEST(TtxAbi, round_trip) {
  printed_count = 0;

  Ttx::Validation::TtxTests::Type::tetrodotoxin_test("Round trip text!"_view);

  ASSERT(printed_count == 2);
  EXPECT_TEXT(printed[0], "Compiler Test"_view);
  EXPECT_TEXT(printed[1], "Round trip text!"_view);
}

PERIMORTEM_UNIT_TEST(TtxAbi, call_result_arithmetic) {
  EXPECT_EQ(Ttx::Validation::TtxTests::Type::arithmetic(Count(7)), Count(24));
}

PERIMORTEM_UNIT_TEST(TtxAbi, register_spill) {
  EXPECT_EQ(
      Ttx::Validation::TtxTests::Type::sum7(
          Count(1), Count(2), Count(3), Count(4), Count(5), Count(6), Count(7)),
      Count(28));
  EXPECT_EQ(
      Ttx::Validation::TtxTests::Type::call_sum7(
          Count(1), Count(2), Count(3), Count(4), Count(5), Count(6), Count(7)),
      Count(28));
  EXPECT_EQ(
      Ttx::Validation::TtxTests::Type::call_real9(
          Real_64(1), Real_64(2), Real_64(3), Real_64(4), Real_64(5),
          Real_64(6), Real_64(7), Real_64(8), Real_64(9)),
      Real_64(45));
}

PERIMORTEM_UNIT_TEST(TtxAbi, scalar_constants) {
  EXPECT(Ttx::Validation::TtxTests::Type::is_even(Count(12)));
  EXPECT_NOT(Ttx::Validation::TtxTests::Type::is_even(Count(13)));
  EXPECT(Ttx::Validation::TtxTests::Type::bool_identity(True));
  EXPECT_NOT(Ttx::Validation::TtxTests::Type::bool_identity(False));
  EXPECT_EQ(Ttx::Validation::TtxTests::Type::negative(), Signed_64(-42));
  EXPECT_EQ(
      Ttx::Validation::TtxTests::Type::signed_half(Signed_64(-9)),
      Signed_64(-4));
  EXPECT_EQ(Ttx::Validation::TtxTests::Type::real_constant(), Real_64(4.25));
  EXPECT_EQ(Ttx::Validation::TtxTests::Type::constant_math(), Count(5));
}

PERIMORTEM_UNIT_TEST(TtxAbi, byte_constants) {
  constexpr Unsigned_8 expected[] = {0xAA, 0xFF, 0x12, 0x45, 0xAC, 0xDE};
  View::Bytes bytes = Ttx::Validation::TtxTests::Type::byte_array();
  ASSERT_EQ(bytes.get_size(), Count(6));
  EXPECT(bytes == View::Bytes(expected, 6));
  EXPECT_TEXT(
      Ttx::Validation::TtxTests::Type::text_literal(), "Raw string"_view);
}

PERIMORTEM_UNIT_TEST(TtxAbi, embedded_file) {
  View::Bytes source = Ttx::Validation::TtxTests::Type::source_file();
  ASSERT(source.get_size() > Count(100));
  EXPECT_TEXT(source.slice(0, 18), "dialect : Library;"_view);
}

PERIMORTEM_UNIT_TEST(TtxAbi, pair_swap) {
  auto pair = Ttx::Validation::TtxTests::Type::swap(Count(17), Count(29));
  EXPECT_EQ(pair.first, Count(29));
  EXPECT_EQ(pair.second, Count(17));
}

PERIMORTEM_UNIT_TEST(TtxAbi, ttx_type_call) {
  EXPECT_EQ(Ttx::Validation::TtxTests::Type::type_call(Count(31)), Count(31));
}

PERIMORTEM_UNIT_TEST(TtxAbi, ttx_addressable_call) {
  EXPECT_EQ(
      Ttx::Validation::TtxTests::Type::addressable_call(Count(37)), Count(37));
}

PERIMORTEM_UNIT_TEST(TtxAbi, cpp_addressable_call) {
  EXPECT_EQ(
      Ttx::Validation::TtxTests::Counter::Addressable::identity(Count(41)),
      Count(41));
}
