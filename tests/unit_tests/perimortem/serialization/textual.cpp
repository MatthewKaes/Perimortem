// Perimortem Engine
// Copyright © Matt Kaes

#include "validation/unit_test.hpp"

#include "perimortem/core/static/vector.hpp"
#include "perimortem/serialization/textual/stream.hpp"
#include "perimortem/utility/null_terminated.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Serialization;

using namespace Validation;

Test::Harness SerializationTextual = {.name = "Serialization::Textual"};

PERIMORTEM_UNIT_TEST(SerializationTextual, simple_text) {
  Static::Bytes<10> buffer;
  Textual::Stream writer(buffer.get_access());

  writer << "Hello"_view << "World"_view;

  EXPECT(writer.is_valid());
  EXPECT_TEXT(buffer.get_view(), "HelloWorld"_view);
}

PERIMORTEM_UNIT_TEST(SerializationTextual, prevent_overflow) {
  Static::Bytes<10> buffer;
  Textual::Stream writer(buffer.get_access());

  writer << "Hello"_view << "World!!!!"_view;

  EXPECT_EQ(writer.is_valid(), false);
  EXPECT_TEXT(buffer.slice(0, 5), "Hello"_view);
}

PERIMORTEM_UNIT_TEST(SerializationTextual, integers) {
  Static::Bytes<12> buffer;
  Textual::Stream writer(buffer.get_access());

  writer << 120000 << -31 << 208;

  EXPECT_EQ(writer.is_valid(), true);
  EXPECT_TEXT(buffer.get_view(), "120000-31208"_view);
}

PERIMORTEM_UNIT_TEST(SerializationTextual, integers_and_text) {
  Static::Bytes<24> buffer;
  Textual::Stream writer(buffer.get_access());

  writer << "Test Value: "_view << 412010 << " units"_view;

  EXPECT_EQ(writer.is_valid(), true);
  EXPECT_TEXT(buffer.get_view(), "Test Value: 412010 units"_view);
}

PERIMORTEM_UNIT_TEST(SerializationTextual, boolean) {
  Static::Bytes<11> buffer;
  Textual::Stream writer(buffer.get_access());

  // C++ will narrow to int and should print 1 and 0
  writer << true << false;

  // Perimortem types print out the full value
  writer << True << False;

  EXPECT_EQ(writer.is_valid(), true);
  EXPECT_TEXT(buffer.get_view(), "10truefalse"_view);
}

PERIMORTEM_UNIT_TEST(SerializationTextual, floats) {
  Static::Bytes<40> buffer;
  Textual::Stream writer(buffer.get_access());

  writer << 12.08 << 0.0000812;

  // Percision test
  writer << Real_32(-2012.78102) << Real_64(-2012.78102);

  EXPECT_EQ(writer.is_valid(), true);
  EXPECT_TEXT(buffer.get_view(), "12.080.0000812-2012.7810058-2012.78102\0\0"_view);
}

PERIMORTEM_UNIT_TEST(SerializationTextual, multiple_writers) {
  Static::Bytes<34> buffer;
  for (Count i = 0; i < buffer.get_size(); i++) {
    buffer[i] = '?';
  }

  Textual::Stream writer_1(buffer.get_access());
  Textual::Stream writer_2(buffer.get_access());

  writer_1 << "Sample text for testing!"_view;
  ASSERT_TEXT(buffer.get_view(), "Sample text for testing!??????????"_view);
  writer_2 << "test: "_view << True;
  ASSERT_TEXT(buffer.get_view(), "test: truet for testing!??????????"_view);
  writer_1 << " "_view << 13891;
  ASSERT_TEXT(buffer.get_view(), "test: truet for testing! 13891????"_view);
  writer_2 << 79.8106;
  ASSERT_TEXT(buffer.get_view(), "test: true79.8106esting! 13891????"_view);
  writer_1 << "Too much text!!"_view;
  ASSERT_TEXT(buffer.get_view(), "test: true79.8106esting! 13891????"_view);

  EXPECT_EQ(writer_1.is_valid(), false);
  EXPECT_EQ(writer_2.is_valid(), true);
}
