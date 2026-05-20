// Perimortem Engine
// Copyright © Matt Kaes

#include "validation/unit_test.hpp"

#include "perimortem/core/null_terminated.hpp"
#include "perimortem/core/reader/textual.hpp"

using namespace Perimortem::Core;
using namespace Validation;

Test::Harness CoreTextualReader = {.name = "Core::Reader::Textual"};

PERIMORTEM_UNIT_TEST(CoreTextualReader, integers) {
  Reader::Textual reader(
      "-1234 5678 -99999 100000 -1234567890123 9876543210"_view);

  auto signed_half = reader.read_int16();
  auto unsigned_half = reader.read_uint16();
  auto signed_int = reader.read_int32();
  auto unsigned_int = reader.read_uint32();
  auto signed_long = reader.read_int64();
  auto unsigned_long = reader.read_uint64();

  EXPECT(reader.is_valid());
  EXPECT_EQ(signed_half, Half(-1234));
  EXPECT_EQ(unsigned_half, UHalf(5678));
  EXPECT_EQ(signed_int, Int(-99999));
  EXPECT_EQ(Long(unsigned_int), Long(100000));
  EXPECT_EQ(signed_long, Long(-1234567890123LL));
  EXPECT_EQ(unsigned_long, ULong(9876543210ULL));
}

PERIMORTEM_UNIT_TEST(CoreTextualReader, integers_and_text) {
  Reader::Textual reader("count: 412010 items"_view);

  auto char_c = reader.read_byte();
  reader.read_byte();  // 'o'
  reader.read_byte();  // 'u'
  reader.read_byte();  // 'n'
  auto char_t = reader.read_byte();
  reader.read_byte();                 // ':'
  auto count = reader.read_uint32();  // skip_whitespace consumes the space

  EXPECT(reader.is_valid());
  EXPECT_EQ(char_c, Byte('c'));
  EXPECT_EQ(char_t, Byte('t'));
  EXPECT_EQ(Long(count), Long(412010));
}

PERIMORTEM_UNIT_TEST(CoreTextualReader, boolean) {
  // C++ native bool narrows to int and prints as 1/0; Perimortem Bool prints
  // "true"/"false". Source is what the writer produces for: true false True
  // False.
  Reader::Textual reader("10truefalse"_view);

  auto native_true_char = reader.read_byte();
  auto native_false_char = reader.read_byte();
  auto peri_true = reader.read_boolean();
  auto peri_false = reader.read_boolean();

  EXPECT(reader.is_valid());
  EXPECT_EQ(native_true_char, Byte('1'));
  EXPECT_EQ(native_false_char, Byte('0'));
  EXPECT(peri_true);
  EXPECT_NOT(peri_false);
}

PERIMORTEM_UNIT_TEST(CoreTextualReader, floats) {
  // Only ".5" fractional values: frac_mult *= 0.1 accumulates rounding error
  // for multi-digit fractions, but 5 * 0.1 = 0.5 exactly in IEEE 754.
  Reader::Textual reader("12.5 -1.5 2.5 0.0"_view);

  auto positive_real = reader.read_real_64();
  auto negative_real = reader.read_real_64();
  auto single_precision = reader.read_real_32();
  auto zero_real = reader.read_real_64();

  EXPECT(reader.is_valid());
  EXPECT_EQ(positive_real, Real_64(12.5));
  EXPECT_EQ(negative_real, Real_64(-1.5));
  EXPECT_EQ(single_precision, Real_32(2.5f));
  EXPECT_EQ(zero_real, Real_64(0.0));
}

PERIMORTEM_UNIT_TEST(CoreTextualReader, prevent_overflow) {
  Reader::Textual reader("Hello"_view);

  for (Count index = 0; index < 5; index++) {
    reader.read_byte();
  }
  EXPECT(reader.is_valid());

  reader.read_byte();
  EXPECT_NOT(reader.is_valid());
}

PERIMORTEM_UNIT_TEST(CoreTextualReader, invalid_bool) {
  Reader::Textual reader("notabool"_view);
  reader.read_boolean();
  EXPECT_NOT(reader.is_valid());
}

PERIMORTEM_UNIT_TEST(CoreTextualReader, set_pointer) {
  Reader::Textual reader("42 99"_view);

  auto first_value = reader.read_int32();
  EXPECT_EQ(first_value, Int(42));

  reader.set_pointer(0);
  auto rewound_value = reader.read_int32();
  EXPECT_EQ(rewound_value, Int(42));
}

PERIMORTEM_UNIT_TEST(CoreTextualReader, multiple_readers) {
  // Both readers operate independently over the same source.
  View::Bytes source = "test: true"_view;
  Reader::Textual readers[] = {
    Reader::Textual(source), Reader::Textual(source)};

  auto first_from_0 = readers[0].read_byte();
  auto first_from_1 = readers[1].read_byte();

  EXPECT_EQ(first_from_0, first_from_1);
  EXPECT_EQ(Long(readers[0].get_location()), Long(1));
  EXPECT_EQ(Long(readers[1].get_location()), Long(1));

  // Advance readers[0] further without moving readers[1].
  readers[0].read_byte();
  readers[0].read_byte();

  EXPECT_EQ(Long(readers[0].get_location()), Long(3));
  EXPECT_EQ(Long(readers[1].get_location()), Long(1));

  // readers[1] catches up by seeking directly to the "true" at position 6.
  readers[1].set_pointer(6);
  auto boolean_from_1 = readers[1].read_boolean();

  EXPECT(readers[1].is_valid());
  EXPECT(boolean_from_1);
}
