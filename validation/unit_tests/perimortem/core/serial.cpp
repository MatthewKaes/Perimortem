// Perimortem Engine
// Copyright © Matt Kaes

#include "perimortem/core/reader/serial.hpp"

#include "validation/unit_test.hpp"

#include "perimortem/core/static/bytes.hpp"
#include "perimortem/core/static/vector.hpp"
#include "perimortem/core/diagnostics/log.hpp"
#include "perimortem/core/null_terminated.hpp"
#include "perimortem/core/writer/serial.hpp"

using namespace Perimortem::Core;
using namespace Validation;

static Diagnostics::Log::Level captured_log_level;
static Static::Bytes<256> captured_log_message;

static auto capture_sink(Diagnostics::Log::Level level, View::Bytes message)
    -> void {
  captured_log_level = level;
  captured_log_message = message;
}

static Harness CoreSerialReader = {
  .name = "Core::Reader::Serial"_view,
  .setup =
      []() {
        Diagnostics::Log::set_sink(capture_sink);
        captured_log_message = ""_view;
      },
  .teardown =
      []() { Diagnostics::Log::set_sink(Diagnostics::Log::default_sink); },
};

PERIMORTEM_UNIT_TEST(CoreSerialReader, unsigned_ints) {
  Reader::Serial reader(
      "\xF5\xAB"                                   // Bits_8(0xAB)
      "\xF6\x34\x12"                               // Bits_16(0x1234)
      "\xF7\x49\x52\x45\x50"                       // Bits_32(0xDEADBEEF)
      "\xF8\xEF\xCD\xAB\x89\x67\x45\x23\x01"_view  // Bits_64(0x0123456789ABCDEF)
  );

  EXPECT(reader.is_valid());
  EXPECT_EQ(reader.read_bits_8(), Bits_8(0xAB));
  EXPECT_EQ(reader.read_bits_16(), Bits_16(0x1234));
  EXPECT_EQ(reader.read_bits_32(), Bits_32('PERI'));
  EXPECT_EQ(reader.read_bits_64(), Bits_64(0x0123456789ABCDEF));

  EXPECT_EQ(reader.read_bits_16(), 0);
  EXPECT_NOT(reader.is_valid());
}

PERIMORTEM_UNIT_TEST(CoreSerialReader, signed_ints) {
  Reader::Serial reader(
      "\xF9\xD6"                                   // SignedBits_8(-42)
      "\xFA\x18\xFC"                               // SignedBits_16(-1000)
      "\xFB\x60\x79\xFE\xFF"                       // SignedBits_32(-100000)
      "\xFC\x00\x36\x65\xC4\xFF\xFF\xFF\xFF"_view  // SignedBits_64(-1000000000)
  );

  EXPECT(reader.is_valid());
  EXPECT_EQ(reader.read_signed_bits_8(), SignedBits_8(-42));
  EXPECT_EQ(reader.read_signed_bits_16(), SignedBits_16(-1000));
  EXPECT_EQ(reader.read_signed_bits_32(), SignedBits_32(-100000));
  EXPECT_EQ(reader.read_signed_bits_64(), SignedBits_64(-1000000000LL));

  EXPECT_EQ(reader.read_signed_bits_32(), 0);
  EXPECT_NOT(reader.is_valid());
}

PERIMORTEM_UNIT_TEST(CoreSerialReader, reals) {
  Reader::Serial reader(
      "\xFD\x00\x00\x40\x40"                       // Real_32(3.0f)
      "\xFE\x00\x00\x00\x00\x00\x00\xF8\x3F"_view  // Real_64(1.5)
  );

  EXPECT(reader.is_valid());
  EXPECT_EQ(reader.read_real_32(), Real_32(3.0f));
  EXPECT_EQ(reader.read_real_64(), Real_64(1.5));
}

PERIMORTEM_UNIT_TEST(CoreSerialReader, blob_small) {
  Reader::Serial reader(
      "\xFF\xF5\xF5\x0D"
      "Hello, World!"_view);

  EXPECT(reader.is_valid());
  EXPECT_TEXT(reader.read_blob(), "Hello, World!"_view);
}

PERIMORTEM_UNIT_TEST(CoreSerialReader, blob_medium) {
  // 300 elements to push the blob to use 16 bit size encoding.
  constexpr Count blob_count = 300;
  Static::Bytes<5 + blob_count> buffer;
  Writer::Serial writer(buffer.get_access());

  Static::Vector<Bits_8, blob_count> source;
  for (Count index = 0; index < blob_count; index++) {
    source[index] = Bits_8(index % 256);
  }
  writer << source.get_view();

  Reader::Serial reader(buffer.get_view());
  auto blob_data = reader.read_blob();

  EXPECT(reader.is_valid());
  EXPECT_EQ(blob_data.get_size(), blob_count);
  EXPECT_EQ(blob_data[0], Byte(0));
  EXPECT_EQ(blob_data[137], Byte(137));
  EXPECT_EQ(blob_data[255], Byte(255));
  EXPECT_EQ(blob_data[256], Byte(0));
}

PERIMORTEM_UNIT_TEST(CoreSerialReader, blob_large) {
  // 17000 elements to push the blob to use 32 bit size encoding.
  constexpr Count blob_count = 17000;
  Static::Bytes<5 + blob_count> buffer;
  Writer::Serial writer(buffer.get_access());

  Static::Vector<Bits_8, blob_count> source;
  for (Count index = 0; index < blob_count; index++) {
    source[index] = Bits_8(index % 256);
  }
  writer << source.get_view();

  Reader::Serial reader(buffer.get_view());
  auto blob_data = reader.read_blob();

  EXPECT(reader.is_valid());
  EXPECT_EQ(blob_data.get_size(), blob_count);
  EXPECT_EQ(blob_data[0], Byte(0));
  EXPECT_EQ(blob_data[blob_count - 1], Byte((blob_count - 1) % 256));
}

constexpr auto file_location_size =
    "[main] validation/unit_tests/perimortem/core/serial.cpp:xxx:xx: "_view
        .get_size() +
    15;

PERIMORTEM_UNIT_TEST(CoreSerialReader, type_mismatch) {
  Reader::Serial reader("\xF7\xAD\xDE\x00\x00"_view);
  auto scope_attribution = Diagnostics::Log::set_attribution();

  EXPECT_EQ(reader.read_bits_16(), 0);
  EXPECT_NOT(reader.is_valid());

  // Make sure message was logged.
  constexpr auto error_message =
      "Mismatched Data Type in serilization stream at position 1. Expected=246, Got=247"_view;
  EXPECT_EQ(UInt(captured_log_level), UInt(Diagnostics::Log::Level::Error));
  EXPECT_TEXT(
      captured_log_message.slice(file_location_size, error_message.get_size()),
      error_message);
}

PERIMORTEM_UNIT_TEST(CoreSerialReader, overflow_read) {
  Reader::Serial reader("\xF5\xFF"_view);
  auto scope_attribution = Diagnostics::Log::set_attribution();

  EXPECT_EQ(reader.read_bits_8(), Bits_8(0xFF));
  EXPECT(reader.is_valid());
  EXPECT_EQ(reader.read_bits_8(), Bits_8(0));
  EXPECT_NOT(reader.is_valid());

  // Make sure message was logged.
  constexpr auto error_message =
      "Serial read over ran buffer at read location 2. source_size=2, read_size=2"_view;
  EXPECT_EQ(UInt(captured_log_level), UInt(Diagnostics::Log::Level::Error));
  EXPECT_TEXT(
      captured_log_message.slice(file_location_size, error_message.get_size()),
      error_message);
}

PERIMORTEM_UNIT_TEST(CoreSerialReader, blob_overrun) {
  Reader::Serial reader(
      "\xFF\xF5\xF5\x12"
      "Hello, World!"_view);
  auto scope_attribution = Diagnostics::Log::set_attribution();

  reader.read_blob();
  EXPECT_NOT(reader.is_valid());

  // Make sure message was logged.
  constexpr auto error_message =
      "Serial read over ran buffer at read location 4. source_size=17, read_size=18"_view;
  EXPECT_EQ(UInt(captured_log_level), UInt(Diagnostics::Log::Level::Error));
  EXPECT_TEXT(
      captured_log_message.slice(file_location_size, error_message.get_size()),
      error_message);
}

PERIMORTEM_UNIT_TEST(CoreSerialReader, blob_bad_size_type) {
  Reader::Serial reader(
      "\xFF\xF5\xAC\x0D"
      "Hello, World!"_view);
  auto scope_attribution = Diagnostics::Log::set_attribution();

  reader.read_blob();
  EXPECT_NOT(reader.is_valid());

  // Make sure message was logged.
  constexpr auto error_message =
      "Serial blob read encountered invalid size encoding type. Expected=245|246|247|248, Got=172"_view;
  EXPECT_EQ(UInt(captured_log_level), UInt(Diagnostics::Log::Level::Error));
  EXPECT_TEXT(
      captured_log_message.slice(file_location_size, error_message.get_size()),
      error_message);
}

PERIMORTEM_UNIT_TEST(CoreSerialReader, blob_bad_data_type) {
  Reader::Serial reader(
      "\xFF\xAC\xF5\x0D"
      "Hello, World!"_view);
  auto scope_attribution = Diagnostics::Log::set_attribution();

  reader.read_blob();
  EXPECT_NOT(reader.is_valid());

  // Make sure message was logged.
  constexpr auto error_message =
      "Mismatched Data Type in serilization stream at position 2. Expected=245, Got=172"_view;
  EXPECT_EQ(UInt(captured_log_level), UInt(Diagnostics::Log::Level::Error));
  EXPECT_TEXT(
      captured_log_message.slice(file_location_size, error_message.get_size()),
      error_message);
}

PERIMORTEM_UNIT_TEST(CoreSerialReader, sequential_mixed) {
  // Separate literals before 'f' and 'h' to stop \x06 greedy hex parsing.
  Reader::Serial reader(
      "\xFF\xF5\xF5\x06"  // "header" blob (4-byte header)
      "header"
      "\xF7\x63\x00\x00\x00"                  // Bits_32(99)
      "\xFC\x0C\xFE\xFF\xFF\xFF\xFF\xFF\xFF"  // SignedBits_64(-500)
      "\xFF\xF5\xF5\x06"                      // "footer" blob (4-byte header)
      "footer"_view);

  EXPECT(reader.is_valid());
  EXPECT_TEXT(reader.read_blob(), "header"_view);
  EXPECT_EQ(reader.read_bits_32(), Bits_32(99));
  EXPECT_EQ(reader.read_signed_bits_64(), SignedBits_32(-500));
  EXPECT_TEXT(reader.read_blob(), "footer"_view);
}

PERIMORTEM_UNIT_TEST(CoreSerialReader, set_pointer) {
  Reader::Serial reader("\xF5\x0A\xF5\x14\xF5\x1E"_view);

  auto first_read = reader.read_bits_8();
  EXPECT_EQ(first_read, Bits_8(10));

  reader.set_pointer(0);
  auto second_read = reader.read_bits_8();
  EXPECT_EQ(second_read, Bits_8(10));
}

static Harness CoreSerialWriter = {
  .name = "Core::Writer::Serial"_view,
};

PERIMORTEM_UNIT_TEST(CoreSerialWriter, unsigned_integers) {
  Static::Bytes<19> buffer;
  Writer::Serial writer(buffer.get_access());

  writer << Bits_8(0xAB) << Bits_16(0x1234) << Bits_32(0xDEADBEEF)
         << Bits_64(0x0123456789ABCDEF);

  EXPECT(writer.is_valid());
  EXPECT_EQ(Long(writer.get_location()), Long(19));
  EXPECT_HEX(
      buffer.slice(0, 19),
      "\xF5\xAB"                                   // Bits_8(0xAB)
      "\xF6\x34\x12"                               // Bits_16(0x1234)
      "\xF7\xEF\xBE\xAD\xDE"                       // Bits_32(0xDEADBEEF)
      "\xF8\xEF\xCD\xAB\x89\x67\x45\x23\x01"_view  // Bits_64(0x0123456789ABCDEF)
  );
}

PERIMORTEM_UNIT_TEST(CoreSerialWriter, signed_integers) {
  Static::Bytes<19> buffer;
  Writer::Serial writer(buffer.get_access());

  writer << SignedBits_8(-42) << SignedBits_16(-1000) << SignedBits_32(-100000)
         << SignedBits_64(-1000000000LL);

  EXPECT(writer.is_valid());
  EXPECT_EQ(Long(writer.get_location()), Long(19));
  EXPECT_HEX(
      buffer.slice(0, 19),
      "\xF9\xD6"                                   // SignedBits_8(-42)
      "\xFA\x18\xFC"                               // SignedBits_16(-1000)
      "\xFB\x60\x79\xFE\xFF"                       // SignedBits_32(-100000)
      "\xFC\x00\x36\x65\xC4\xFF\xFF\xFF\xFF"_view  // SignedBits_64(-1000000000)
  );
}

PERIMORTEM_UNIT_TEST(CoreSerialWriter, endian_layout) {
  Static::Bytes<5> buffer;
  Writer::Serial writer(buffer.get_access());

  writer << Bits_32(0x12345678);

  EXPECT(writer.is_valid());
  EXPECT_EQ(buffer[0], Byte(Writer::Serial::DataType::Bits_32));
  EXPECT_EQ(buffer[1], Byte(0x78));
  EXPECT_EQ(buffer[2], Byte(0x56));
  EXPECT_EQ(buffer[3], Byte(0x34));
  EXPECT_EQ(buffer[4], Byte(0x12));
}

PERIMORTEM_UNIT_TEST(CoreSerialWriter, reals) {
  // Real_32(3.0f) = 0x40400000; Real_64(1.5) = 0x3FF8000000000000.
  Static::Bytes<14> buffer;
  Writer::Serial writer(buffer.get_access());

  writer << Real_32(3.0f) << Real_64(1.5);

  EXPECT(writer.is_valid());
  EXPECT_HEX(
      buffer.slice(0, 14),
      "\xFD\x00\x00\x40\x40"                       // Real_32(3.0f)
      "\xFE\x00\x00\x00\x00\x00\x00\xF8\x3F"_view  // Real_64(1.5)
  );
}

PERIMORTEM_UNIT_TEST(CoreSerialWriter, blob) {
  // Blob format: Blob_tag + Bits_8_element_tag + size_type_tag + size_value +
  // data. 9 elements fits in Bits_8, so header is 4 bytes.
  Static::Bytes<13> buffer;
  Writer::Serial writer(buffer.get_access());

  writer << "blob data"_view;

  EXPECT(writer.is_valid());
  // Separate literals so \x09 terminates before 'b' (a hex digit) is seen.
  EXPECT_HEX(
      buffer.slice(0, 13),
      "\xFF\xF5\xF5"
      "\x09"
      "blob data"_view);
}

PERIMORTEM_UNIT_TEST(CoreSerialWriter, blob_medium_size) {
  // 300 elements: size > 255, so the size is encoded as Bits_16.
  // Header: Blob_tag(1) + Bits_8_element(1) + Bits_16_size_tag(1) + 300 LE(2) =
  // 5 bytes.
  constexpr Count blob_count = 300;
  Static::Bytes<5 + blob_count> buffer;
  Writer::Serial writer(buffer.get_access());

  Static::Vector<Bits_8, blob_count> source;
  for (Count index = 0; index < blob_count; index++) {
    source[index] = Bits_8(index % 256);
  }
  writer << source.get_view();

  EXPECT(writer.is_valid());
  EXPECT_EQ(buffer[0], Byte(Writer::Serial::DataType::Blob));
  EXPECT_EQ(buffer[1], Byte(Writer::Serial::DataType::Bits_8));
  EXPECT_EQ(buffer[2], Byte(Writer::Serial::DataType::Bits_16));
  // 300 = 0x012C in LE
  EXPECT_EQ(buffer[3], Byte(0x2C));
  EXPECT_EQ(buffer[4], Byte(0x01));
  EXPECT_EQ(Long(writer.get_location()), Long(5 + blob_count));
}

PERIMORTEM_UNIT_TEST(CoreSerialWriter, blob_large_size) {
  // 17000 elements: size > 16384, but still fits in Bits_16 (max 65535).
  // Header: Blob_tag(1) + Bits_8_element(1) + Bits_16_size_tag(1) + 17000 LE(2)
  // = 5 bytes.
  constexpr Count blob_count = 17000;
  Static::Bytes<5 + blob_count> buffer;
  Writer::Serial writer(buffer.get_access());

  Static::Vector<Bits_8, blob_count> source;
  for (Count index = 0; index < blob_count; index++) {
    source[index] = Bits_8(index % 256);
  }
  writer << source.get_view();

  EXPECT(writer.is_valid());
  EXPECT_EQ(buffer[0], Byte(Writer::Serial::DataType::Blob));
  EXPECT_EQ(buffer[1], Byte(Writer::Serial::DataType::Bits_8));
  EXPECT_EQ(buffer[2], Byte(Writer::Serial::DataType::Bits_16));
  // 17000 = 0x4268 in LE
  EXPECT_EQ(buffer[3], Byte(0x68));
  EXPECT_EQ(buffer[4], Byte(0x42));
  EXPECT_EQ(Long(writer.get_location()), Long(5 + blob_count));
}

PERIMORTEM_UNIT_TEST(CoreSerialWriter, overflow) {
  Static::Bytes<4> buffer;
  Writer::Serial writer(buffer.get_access());

  writer << Bits_32(0xDEAD);

  EXPECT_NOT(writer.is_valid());
}

PERIMORTEM_UNIT_TEST(CoreSerialWriter, set_pointer) {
  Static::Bytes<8> buffer;
  Writer::Serial writer(buffer.get_access());

  writer << Bits_8(10) << Bits_8(20);
  EXPECT_EQ(Long(writer.get_location()), Long(4));

  writer.set_pointer(0);
  writer << Bits_8(30);

  EXPECT(writer.is_valid());
  // Position 0-1 holds Bits_8(30); position 2-3 holds the original Bits_8(20).
  EXPECT_HEX(buffer.slice(0, 4), "\xF5\x1E\xF5\x14"_view);
}

PERIMORTEM_UNIT_TEST(CoreSerialWriter, multiple_writers) {
  Static::Bytes<8> buffer;
  Writer::Serial writers[] = {
    Writer::Serial(buffer.get_access()),
    Writer::Serial(buffer.get_access()),
  };

  writers[0] << Bits_8(10);
  writers[1] << Bits_8(20);  // Overwrites writers[0]'s bytes at position 0.
  writers[0] << Bits_8(30);  // writers[0] is at position 2, writes there.

  EXPECT(writers[0].is_valid());
  EXPECT(writers[1].is_valid());
  EXPECT_HEX(buffer.slice(0, 4), "\xF5\x14\xF5\x1E"_view);
}
