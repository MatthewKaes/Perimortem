// Perimortem Engine
// Copyright © Matt Kaes

#include "perimortem/core/reader/textual.hpp"

#include "validation/unit_test.hpp"

#include "perimortem/core/null_terminated.hpp"
#include "perimortem/core/writer/textual.hpp"

using namespace Perimortem::Core;
using namespace Validation;

static Harness CoreTextualReader = {
  .name = "Core::Reader::Textual"_view,
};

PERIMORTEM_UNIT_TEST(CoreTextualReader, integers) {
  Reader::Textual reader(
      "-1234 5678 -99999 100000 -1234567890123 9876543210"_view);

  EXPECT(reader.is_valid());
  EXPECT_EQ(reader.read_half(), Half(-1234));
  EXPECT_EQ(reader.read_unsigned_half(), UHalf(5678));
  EXPECT_EQ(reader.read_int(), Int(-99999));
  EXPECT_EQ(reader.read_unsigned_int(), UInt(100000));
  EXPECT_EQ(reader.read_long(), Long(-1234567890123LL));
  EXPECT_EQ(reader.read_unsigned_long(), ULong(9876543210ULL));
}

PERIMORTEM_UNIT_TEST(CoreTextualReader, integers_and_text) {
  Reader::Textual reader("count: 412010 items"_view);

  EXPECT_EQ(reader.read_byte(), Byte('c'));
  reader.read_byte();  // 'o'
  reader.read_byte();  // 'u'
  reader.read_byte();  // 'n'
  EXPECT_EQ(reader.read_byte(), Byte('t'));
  reader.read_byte();  // ':'
  EXPECT_EQ(reader.read_unsigned_int(), Int(412010));
  EXPECT(reader.is_valid());
}

PERIMORTEM_UNIT_TEST(CoreTextualReader, boolean) {
  // C++ native bool narrows to int and prints as 1/0 so make sure we test for
  // that for ABI reasons, and use Bool (proper ABI) for "true"/"false".
  Reader::Textual reader("10truefalseTrueFalse"_view);

  EXPECT_EQ(reader.read_byte(), Byte('1'));
  EXPECT_EQ(reader.read_byte(), Byte('0'));
  EXPECT(reader.read_flag());
  EXPECT_NOT(reader.read_flag());
  EXPECT(reader.read_flag());
  EXPECT_NOT(reader.read_flag());
  EXPECT(reader.is_valid());
}

PERIMORTEM_UNIT_TEST(CoreTextualReader, floats) {
  Reader::Textual reader("12.5 -1.5 2.512 0.0"_view);

  EXPECT_EQ(reader.read_real_64(), Real_64(12.5));
  EXPECT_EQ(reader.read_real_64(), Real_64(-1.5));
  EXPECT_EQ(reader.read_real_32(), Real_32(2.512f));
  EXPECT_EQ(reader.read_real_64(), Real_64(0.0));
  EXPECT(reader.is_valid());
}

PERIMORTEM_UNIT_TEST(CoreTextualReader, prevent_overflow) {
  Reader::Textual reader("Hello"_view);

  for (Count i = 0; i < 5; i++) {
    reader.read_byte();
  }
  EXPECT(reader.is_valid());

  reader.read_byte();
  EXPECT_NOT(reader.is_valid());
}

PERIMORTEM_UNIT_TEST(CoreTextualReader, invalid_bool) {
  Reader::Textual reader("badflag"_view);
  reader.read_flag();
  EXPECT_NOT(reader.is_valid());
}

PERIMORTEM_UNIT_TEST(CoreTextualReader, just_whitespace) {
  Reader::Textual reader("      "_view);

  reader.read_byte();
  EXPECT(reader.is_valid());
  reader.reset();
  reader.read_flag();
  EXPECT_NOT(reader.is_valid());
  reader.reset();
  reader.read_half();
  EXPECT_NOT(reader.is_valid());
  reader.reset();
  reader.read_unsigned_half();
  EXPECT_NOT(reader.is_valid());
  reader.reset();
  reader.read_int();
  EXPECT_NOT(reader.is_valid());
  reader.reset();
  reader.read_unsigned_int();
  EXPECT_NOT(reader.is_valid());
  reader.reset();
  reader.read_long();
  EXPECT_NOT(reader.is_valid());
  reader.reset();
  reader.read_unsigned_long();
  EXPECT_NOT(reader.is_valid());
  reader.reset();
  reader.read_real_32();
  EXPECT_NOT(reader.is_valid());
  reader.reset();
  reader.read_real_64();
  EXPECT_NOT(reader.is_valid());
}

PERIMORTEM_UNIT_TEST(CoreTextualReader, set_pointer) {
  Reader::Textual reader("42 99"_view);

  EXPECT_EQ(reader.read_int(), Int(42));
  EXPECT_EQ(reader.read_int(), Int(99));
  EXPECT(reader.is_valid());

  reader.set_pointer(0);
  EXPECT_EQ(reader.read_int(), Int(42));
  EXPECT_EQ(reader.read_int(), Int(99));
  EXPECT(reader.is_valid());
}

PERIMORTEM_UNIT_TEST(CoreTextualReader, multiple_readers) {
  // Both readers operate independently over the same source.
  View::Bytes source = "a12 true"_view;
  Reader::Textual readers[] = {
    Reader::Textual(source), Reader::Textual(source)};

  EXPECT_EQ(readers[0].read_byte(), readers[1].read_byte());
  EXPECT_EQ(readers[0].get_location(), Count(1));
  EXPECT_EQ(readers[1].get_location(), Count(1));

  EXPECT_EQ(readers[0].read_int(), 12);
  EXPECT_EQ(readers[0].read_flag(), true);

  EXPECT(readers[0].is_valid());
  EXPECT(readers[1].is_valid());

  EXPECT_EQ(readers[1].read_unsigned_int(), 12);
  EXPECT_EQ(readers[1].read_flag(), true);

  EXPECT(readers[0].is_valid());
  EXPECT(readers[1].is_valid());
}

static Harness CoreTextual = {
  .name = "Core::Writer::Textual"_view,
};

PERIMORTEM_UNIT_TEST(CoreTextual, simple_text) {
  Static::Bytes<10> buffer;
  Writer::Textual writer(buffer.get_access());

  writer << "Hello"_view << "World"_view;

  EXPECT(writer.is_valid());
  EXPECT_TEXT(buffer.get_view(), "HelloWorld"_view);
}

PERIMORTEM_UNIT_TEST(CoreTextual, prevent_overflow) {
  Static::Bytes<10> buffer;
  Writer::Textual writer(buffer.get_access());

  writer << "Hello"_view << "World!!!!"_view;

  EXPECT_NOT(writer.is_valid());
  EXPECT_TEXT(buffer.slice(0, 5), "Hello"_view);
}

PERIMORTEM_UNIT_TEST(CoreTextual, integers) {
  Static::Bytes<12> buffer;
  Writer::Textual writer(buffer.get_access());

  writer << 120000 << -31 << 208;

  EXPECT(writer.is_valid());
  EXPECT_TEXT(buffer.get_view(), "120000-31208"_view);
}

PERIMORTEM_UNIT_TEST(CoreTextual, integers_and_text) {
  Static::Bytes<24> buffer;
  Writer::Textual writer(buffer.get_access());

  writer << "Test Value: "_view << 412010 << " units"_view;

  EXPECT(writer.is_valid());
  EXPECT_TEXT(buffer.get_view(), "Test Value: 412010 units"_view);
}

PERIMORTEM_UNIT_TEST(CoreTextual, boolean) {
  Static::Bytes<11> buffer;
  Writer::Textual writer(buffer.get_access());

  // C++ will narrow to int and should print 1 and 0
  writer << true << false;

  // Perimortem types print out the full value
  writer << True << False;

  EXPECT(writer.is_valid());
  EXPECT_TEXT(buffer.get_view(), "10truefalse"_view);
}

PERIMORTEM_UNIT_TEST(CoreTextual, floats) {
  Static::Bytes<40> buffer;
  Writer::Textual writer(buffer.get_access());

  writer << 12.08 << 0.0000812;

  // Percision test
  writer << Real_32(-2012.78102) << Real_64(-2012.78102);

  EXPECT(writer.is_valid());
  EXPECT_TEXT(
      buffer.get_view(), "12.080.0000812-2012.7810058-2012.78102\0\0"_view);
}

PERIMORTEM_UNIT_TEST(CoreTextual, multiple_writers) {
  Static::Bytes<34> buffer;
  for (Count index = 0; index < buffer.get_size(); index++) {
    buffer[index] = '?';
  }

  Writer::Textual writers[] = {
    Writer::Textual(buffer.get_access()),
    Writer::Textual(buffer.get_access()),
  };

  writers[0] << "Sample text for testing!"_view;
  ASSERT_TEXT(buffer.get_view(), "Sample text for testing!??????????"_view);
  writers[1] << "test: "_view << True;
  ASSERT_TEXT(buffer.get_view(), "test: truet for testing!??????????"_view);
  writers[0] << " "_view << 13891;
  ASSERT_TEXT(buffer.get_view(), "test: truet for testing! 13891????"_view);
  writers[1] << 79.8106;
  ASSERT_TEXT(buffer.get_view(), "test: true79.8106esting! 13891????"_view);
  writers[0] << "Too much text!!"_view;
  ASSERT_TEXT(buffer.get_view(), "test: true79.8106esting! 13891????"_view);

  EXPECT_NOT(writers[0].is_valid());
  EXPECT(writers[1].is_valid());
}
