// Perimortem Engine
// Copyright © Matt Kaes

#include "perimortem/core/writer/textual.hpp"

#include "validation/unit_test.hpp"

#include "perimortem/core/static/vector.hpp"
#include "perimortem/core/null_terminated.hpp"

using namespace Perimortem::Core;

using namespace Validation;

Test::Harness CoreTextual = {.name = "Core::Textual"};

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
  for (Count i = 0; i < buffer.get_size(); i++) {
    buffer[i] = '?';
  }

  Writer::Textual writer_1(buffer.get_access());
  Writer::Textual writer_2(buffer.get_access());

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

  EXPECT_NOT(writer_1.is_valid());
  EXPECT(writer_2.is_valid());
}
