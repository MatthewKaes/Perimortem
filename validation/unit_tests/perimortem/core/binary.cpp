// Perimortem Engine
// Copyright © Matt Kaes

#include "perimortem/core/reader/binary.hpp"

#include "validation/unit_test.hpp"

#include "perimortem/core/static/bytes.hpp"
#include "perimortem/core/diagnostics/log.hpp"
#include "perimortem/core/null_terminated.hpp"
#include "perimortem/core/writer/binary.hpp"

using namespace Perimortem::Core;
using namespace Validation;

static Diagnostics::Log::Level log_level;
static Static::Bytes<256> log_message;

static auto capture_sink(Diagnostics::Log::Level level, View::Bytes message)
    -> void {
  log_level = level;
  log_message = message;
}

static Harness CoreBinaryReader = {
  .name = "Core::Reader::Binary"_view,
  .setup =
      []() {
        Diagnostics::Log::set_sink(capture_sink);
        log_message = ""_view;
      },
  .teardown =
      []() { Diagnostics::Log::set_sink(Diagnostics::Log::default_sink); },
};

PERIMORTEM_UNIT_TEST(CoreBinaryReader, little_endian_unsigned) {
  using Reader = Reader::Binary<Data::ByteOrder::Little>;
  // Make sure block is aligned on the stack.
  alignas(8) Static::Bytes<24> aligned_source(
      "\xAB\x00\x00\x00"                  // Read 1 byte + 3 byte padding.
      "IREP"                              // Read 4 bytes
      "\x34\x12\x00\x00\x00\x00\x00\x00"  // Read 2 bytes + 6 bytes padding.
      "\xEF\xCD\xAB\x89\x67\x45\x23\x01"_view  // Read the final 8 bytes.
  );
  Reader reader(aligned_source);

  EXPECT_EQ(reader.read_bits_8(), Bits_8(0xAB));
  EXPECT_EQ(reader.read_bits_32(), Bits_32('PERI'));
  EXPECT_EQ(reader.read_bits_16(), Bits_16(0x1234));
  EXPECT_EQ(reader.read_bits_64(), Bits_64(0x0123456789ABCDEF));
  EXPECT(reader.is_valid());
}

PERIMORTEM_UNIT_TEST(CoreBinaryReader, little_endian_signed) {
  using Reader = Reader::Binary<Data::ByteOrder::Little>;
  // Make sure block is aligned on the stack.
  alignas(8) Static::Bytes<16> aligned_source(
      "\xD6\x00"
      "\x18\xFC"
      "\x60\x79\xFE\xFF"
      "\x00\x36\x65\xC4\xFF\xFF\xFF\xFF"_view);
  Reader reader(aligned_source);

  EXPECT_EQ(reader.read_signed_bits_8(), SignedBits_8(-42));
  EXPECT_EQ(reader.read_signed_bits_16(), SignedBits_16(-1000));
  EXPECT_EQ(reader.read_signed_bits_32(), SignedBits_32(-100000));
  EXPECT_EQ(reader.read_signed_bits_64(), SignedBits_64(-1000000000LL));
  EXPECT(reader.is_valid());
}

PERIMORTEM_UNIT_TEST(CoreBinaryReader, little_endian_reals) {
  using Reader = Reader::Binary<Data::ByteOrder::Little>;
  alignas(8) Static::Bytes<16> aligned_source(
      "\x00\x00\x40\x40"                       // Real_32
      "\x00\x00\x00\x00"                       // 4 byte padding
      "\x00\x00\x00\x00\x00\x00\xF8\x3F"_view  // Real_64
  );
  Reader reader(aligned_source);

  EXPECT_EQ(reader.read_real_32(), Real_32(3.0f));
  EXPECT_EQ(reader.read_real_64(), Real_64(1.5));
  EXPECT(reader.is_valid());
}

PERIMORTEM_UNIT_TEST(CoreBinaryReader, big_endian_unsigned) {
  using Reader = Reader::Binary<Data::ByteOrder::Big>;
  // Make sure block is aligned on the stack.
  alignas(8) Static::Bytes<24> aligned_source(
      "\xAB\x00\x00\x00"                  // Read 1 byte + 3 byte padding.
      "PERI"                              // Read 4 bytes
      "\x12\x34\x00\x00\x00\x00\x00\x00"  // Read 2 bytes + 6 bytes padding.
      "\x01\x23\x45\x67\x89\xAB\xCD\xEF"_view  // Read the final 8 bytes.
  );
  Reader reader(aligned_source);

  EXPECT_EQ(reader.read_bits_8(), Bits_8(0xAB));
  EXPECT_EQ(reader.read_bits_32(), Bits_32('PERI'));
  EXPECT_EQ(reader.read_bits_16(), Bits_16(0x1234));
  EXPECT_EQ(reader.read_bits_64(), Bits_64(0x0123456789ABCDEF));
  EXPECT(reader.is_valid());
}

PERIMORTEM_UNIT_TEST(CoreBinaryReader, big_endian_signed) {
  using Reader = Reader::Binary<Data::ByteOrder::Big>;
  // Make sure block is aligned on the stack.
  alignas(8) Static::Bytes<16> aligned_source(
      "\xD6\x00"
      "\xFC\x18"
      "\xFF\xFE\x79\x60"
      "\xFF\xFF\xFF\xFF\xC4\x65\x36\x00"_view);
  Reader reader(aligned_source);

  EXPECT_EQ(reader.read_signed_bits_8(), SignedBits_8(-42));
  EXPECT_EQ(reader.read_signed_bits_16(), SignedBits_16(-1000));
  EXPECT_EQ(reader.read_signed_bits_32(), SignedBits_32(-100000));
  EXPECT_EQ(reader.read_signed_bits_64(), SignedBits_64(-1000000000LL));
  EXPECT(reader.is_valid());
}

PERIMORTEM_UNIT_TEST(CoreBinaryReader, big_endian_reals) {
  using Reader = Reader::Binary<Data::ByteOrder::Big>;
  alignas(8) Static::Bytes<16> aligned_source(
      "\x40\x40\x00\x00\x00\x00\x00\x00"       // Real_32 + 4 byte padding
      "\x3F\xF8\x00\x00\x00\x00\x00\x00"_view  // Real_64
  );
  Reader reader(aligned_source);

  EXPECT_EQ(reader.read_real_32(), Real_32(3.0f));
  EXPECT_EQ(reader.read_real_64(), Real_64(1.5));
  EXPECT(reader.is_valid());
}

PERIMORTEM_UNIT_TEST(CoreBinaryReader, raw_bytes) {
  using Reader = Reader::Binary<Data::ByteOrder::Big>;
  Reader reader("Hello, World!"_view);

  EXPECT_TEXT(reader.read_bytes(5), "Hello"_view);
  EXPECT_EQ(reader.get_location(), Count(5));
  reader.read_bytes(2);
  EXPECT_TEXT(reader.read_bytes(5), "World"_view);
  reader.read_bytes(12);
  EXPECT_NOT(reader.is_valid());
}

constexpr auto file_location_size =
    "[main] validation/unit_tests/perimortem/core/binary.cpp:xxx:xx: "_view
        .get_size() +
    15;

PERIMORTEM_UNIT_TEST(CoreBinaryReader, overflow_read) {
  using Reader = Reader::Binary<Data::ByteOrder::Little>;
  Reader reader("\xAB\xCD"_view);
  auto scope_attribution = Diagnostics::Log::set_attribution();

  // Out of bounds read should return null and set the reader to invalid.
  EXPECT_EQ(reader.read_bits_32(), Bits_32(0));
  EXPECT_NOT(reader.is_valid());

  // Make sure message was logged.
  constexpr auto error_message =
      "Binary read over ran buffer at read location 0. source_size=2, read_size=4"_view;
  EXPECT_EQ(UInt(log_level), UInt(Diagnostics::Log::Level::Error));
  EXPECT_TEXT(
      log_message.slice(file_location_size, error_message.get_size()),
      error_message);
}

PERIMORTEM_UNIT_TEST(CoreBinaryReader, set_pointer) {
  using Reader = Reader::Binary<Data::ByteOrder::Little>;
  alignas(alignof(Bits_16)) Static::Bytes<6> aligned_source(
      "\x0A\x00\x14\x00\x1E\x00"_view);
  Reader reader(aligned_source);

  EXPECT_EQ(reader.read_bits_16(), Bits_16(0x0A));

  // Read from an incorrect offset which should algin to position 2.
  reader.set_pointer(1);
  EXPECT_EQ(reader.read_bits_16(), Bits_16(0x14));
}

PERIMORTEM_UNIT_TEST(CoreBinaryReader, multiple_readers) {
  using Reader = Reader::Binary<Data::ByteOrder::Little>;
  alignas(alignof(Bits_16)) Static::Bytes<6> aligned_source(
      "\x01\x00\x02\x00\x03\x00"_view);
  Reader readers[] = {Reader(aligned_source), Reader(aligned_source)};

  EXPECT_EQ(readers[0].read_bits_16(), readers[1].read_bits_16());
  EXPECT_EQ(Long(readers[0].get_location()), Long(2));
  EXPECT_EQ(Long(readers[1].get_location()), Long(2));

  EXPECT_EQ(readers[0].read_bits_16(), Bits_16(0x02));
  EXPECT_EQ(Long(readers[0].get_location()), Long(4));
  EXPECT_EQ(Long(readers[1].get_location()), Long(2));
}

static Harness CoreBinaryWriter = {
  .name = "Core::Writer::Binary"_view,
};

PERIMORTEM_UNIT_TEST(CoreBinaryWriter, little_endian_unsigned) {
  using Writer = Writer::Binary<Data::ByteOrder::Little>;
  alignas(8) Static::Bytes<16> buffer;
  Writer writer(buffer);

  writer << Bits_8(0xAB) << Bits_16(0x1234) << Bits_32('PERI')
         << Bits_64(0x0123456789ABCDEF);

  EXPECT(writer.is_valid());
  EXPECT_EQ(Long(writer.get_location()), Long(16));
  EXPECT_HEX(
      buffer,
      "\xAB\x00"
      "\x34\x12"
      "IREP"
      "\xEF\xCD\xAB\x89\x67\x45\x23\x01"_view);
}

PERIMORTEM_UNIT_TEST(CoreBinaryWriter, little_endian_signed) {
  using Writer = Writer::Binary<Data::ByteOrder::Little>;
  alignas(8) Static::Bytes<16> buffer;
  Writer writer(buffer);

  writer << SignedBits_8(-42) << SignedBits_16(-1000) << SignedBits_32(-100000)
         << SignedBits_64(-1000000000LL);

  EXPECT(writer.is_valid());
  EXPECT_HEX(
      buffer,
      "\xD6\x00"
      "\x18\xFC"
      "\x60\x79\xFE\xFF"
      "\x00\x36\x65\xC4\xFF\xFF\xFF\xFF"_view);
}

PERIMORTEM_UNIT_TEST(CoreBinaryWriter, little_endian_reals) {
  // Real_32(3.0f) = 0x40400000; Real_64(1.5) = 0x3FF8000000000000.
  using Writer = Writer::Binary<Data::ByteOrder::Little>;
  alignas(8) Static::Bytes<16> buffer;
  Writer writer(buffer);

  writer << Real_32(3.0f) << Real_64(1.5);

  EXPECT(writer.is_valid());
  EXPECT_HEX(
      buffer,
      "\x00\x00\x40\x40"
      "\x00\x00\x00\x00"
      "\x00\x00\x00\x00\x00\x00\xF8\x3F"_view);
}

PERIMORTEM_UNIT_TEST(CoreBinaryWriter, big_endian_unsigned) {
  using Writer = Writer::Binary<Data::ByteOrder::Big>;
  alignas(8) Static::Bytes<16> buffer;
  Writer writer(buffer);

  writer << Bits_8(0xAB) << Bits_16(0x1234) << Bits_32('PERI')
         << Bits_64(0x0123456789ABCDEF);

  EXPECT(writer.is_valid());
  EXPECT_EQ(Long(writer.get_location()), Long(16));
  EXPECT_HEX(
      buffer,
      "\xAB\x00"
      "\x12\x34"
      "PERI"
      "\x01\x23\x45\x67\x89\xAB\xCD\xEF"_view);
}

PERIMORTEM_UNIT_TEST(CoreBinaryWriter, big_endian_signed) {
  using Writer = Writer::Binary<Data::ByteOrder::Big>;
  alignas(8) Static::Bytes<16> buffer;
  Writer writer(buffer);

  writer << SignedBits_8(-42) << SignedBits_16(-1000) << SignedBits_32(-100000)
         << SignedBits_64(-1000000000LL);

  EXPECT(writer.is_valid());
  EXPECT_HEX(
      buffer,
      "\xD6\x00"
      "\xFC\x18"
      "\xFF\xFE\x79\x60"
      "\xFF\xFF\xFF\xFF\xC4\x65\x36\x00"_view);
}

PERIMORTEM_UNIT_TEST(CoreBinaryWriter, big_endian_reals) {
  // Real_32(3.0f) = 0x40400000; Real_64(1.5) = 0x3FF8000000000000.
  using Writer = Writer::Binary<Data::ByteOrder::Big>;
  alignas(8) Static::Bytes<16> buffer;
  Writer writer(buffer);

  writer << Real_32(3.0f) << Real_64(1.5);

  EXPECT(writer.is_valid());
  EXPECT_HEX(
      buffer,
      "\x40\x40\x00\x00"
      "\x00\x00\x00\x00"
      "\x3F\xF8\x00\x00\x00\x00\x00\x00"_view);
}

PERIMORTEM_UNIT_TEST(CoreBinaryWriter, raw_bytes) {
  using Writer = Writer::Binary<Data::ByteOrder::Native>;
  Static::Bytes<9> buffer;
  Writer writer(buffer);

  writer << "blob data"_view;

  EXPECT(writer.is_valid());
  EXPECT_TEXT(buffer.slice(0, 9), "blob data"_view);
}

PERIMORTEM_UNIT_TEST(CoreBinaryWriter, overflow) {
  using Writer = Writer::Binary<Data::ByteOrder::Native>;
  Static::Bytes<3> buffer;
  Writer writer(buffer);

  writer << Bits_32('PERI');

  EXPECT_NOT(writer.is_valid());
}

PERIMORTEM_UNIT_TEST(CoreBinaryWriter, set_pointer) {
  using Writer = Writer::Binary<Data::ByteOrder::Little>;
  alignas(alignof(Bits_16)) Static::Bytes<6> buffer;
  Writer writer(buffer);

  writer << Bits_16(0x0A0B) << Bits_16(0x0C0D);
  EXPECT_EQ(Long(writer.get_location()), Long(4));

  writer.set_pointer(0);
  writer << Bits_16(0x1234);

  EXPECT(writer.is_valid());
  EXPECT_HEX(
      buffer,
      "\x34\x12"
      "\x0D\x0C\0\0"_view);
}

PERIMORTEM_UNIT_TEST(CoreBinaryWriter, multiple_writers) {
  using Writer = Writer::Binary<Data::ByteOrder::Little>;
  alignas(alignof(Bits_16)) Static::Bytes<4> buffer;
  Writer writers[] = {
    Writer(buffer),
    Writer(buffer),
  };

  writers[0] << Bits_16(0xAAAA);
  writers[1] << Bits_16(0xBBBB);  // overwrites writers[0] at position 0
  writers[0] << Bits_16(0xCCCC);  // writers[0] is now at position 2

  EXPECT(writers[0].is_valid());
  EXPECT(writers[1].is_valid());
  EXPECT_HEX(buffer, "\xBB\xBB\xCC\xCC"_view);
}
