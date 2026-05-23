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

PERIMORTEM_UNIT_TEST(CoreBinaryReader, little_endian_unsigned_integers) {
  using LittleBinary = Reader::Binary<Data::ByteOrder::Little>;
  // Make sure block is aligned on the stack.
  alignas(alignof(Bits_64)) Static::Bytes<24> aligned_source(
      "\xAB\x00\x00\x00"                  // Read 1 byte + 3 byte padding.
      "\x49\x52\x45\x50"                  // Read 4 bytes
      "\x34\x12\x00\x00\x00\x00\x00\x00"  // Read 2 bytes + 6 bytes padding.
      "\xEF\xCD\xAB\x89\x67\x45\x23\x01"_view  // Read the final 8 bytes.
  );
  LittleBinary reader(aligned_source.get_view());

  EXPECT(reader.is_valid());
  EXPECT_EQ(reader.read_bits_8(), Bits_8(0xAB));
  EXPECT_EQ(reader.read_bits_32(), Bits_32('PERI'));
  EXPECT_EQ(reader.read_bits_16(), Bits_16(0x1234));
  EXPECT_EQ(reader.read_bits_64(), Bits_64(0x0123456789ABCDEF));
}

PERIMORTEM_UNIT_TEST(CoreBinaryReader, little_endian_signed_integers) {
  using LittleBinary = Reader::Binary<Data::ByteOrder::Little>;
  // Make sure block is aligned on the stack.
  alignas(alignof(Bits_64)) Static::Bytes<16> aligned_source(
      "\xD6\x00"                               // SignedBits_8(-42) + pad
      "\x18\xFC"                               // SignedBits_16(-1000) LE
      "\x60\x79\xFE\xFF"                       // SignedBits_32(-100000) LE
      "\x00\x36\x65\xC4\xFF\xFF\xFF\xFF"_view  // SignedBits_64(-1000000000) LE
  );
  LittleBinary reader(aligned_source.get_view());

  EXPECT(reader.is_valid());
  EXPECT_EQ(reader.read_signed_bits_8(), SignedBits_8(-42));
  EXPECT_EQ(reader.read_signed_bits_16(), SignedBits_16(-1000));
  EXPECT_EQ(reader.read_signed_bits_32(), SignedBits_32(-100000));
  EXPECT_EQ(reader.read_signed_bits_64(), SignedBits_64(-1000000000LL));
}

PERIMORTEM_UNIT_TEST(CoreBinaryReader, big_endian_layout) {
  using BigBinary = Reader::Binary<Data::ByteOrder::Big>;
  alignas(alignof(Bits_32)) Static::Bytes<4> aligned_source(
      "\x12\x34\x56\x78"_view);
  BigBinary reader(aligned_source.get_view());

  auto value = reader.read_bits_32();

  EXPECT(reader.is_valid());
  EXPECT_EQ(Long(value), Long(0x12345678));
}

PERIMORTEM_UNIT_TEST(CoreBinaryReader, little_endian_reals) {
  using LittleBinary = Reader::Binary<Data::ByteOrder::Little>;
  alignas(alignof(Bits_64)) Static::Bytes<16> aligned_source(
      "\x00\x00\x40\x40\x00\x00\x00\x00"       // Real_32 + 4 byte padding
      "\x00\x00\x00\x00\x00\x00\xF8\x3F"_view  // Real_64
  );
  LittleBinary reader(aligned_source.get_view());

  EXPECT(reader.is_valid());
  EXPECT_EQ(reader.read_real_32(), Real_32(3.0f));
  EXPECT_EQ(reader.read_real_64(), Real_64(1.5));
}

PERIMORTEM_UNIT_TEST(CoreBinaryReader, raw_bytes) {
  using LittleBinary = Reader::Binary<Data::ByteOrder::Little>;
  LittleBinary reader("Hello, World!"_view);

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
  using LittleBinary = Reader::Binary<Data::ByteOrder::Little>;
  LittleBinary reader("\xAB\xCD"_view);
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
  using LittleBinary = Reader::Binary<Data::ByteOrder::Little>;
  alignas(alignof(Bits_16)) Static::Bytes<6> aligned_source(
      "\x0A\x00\x14\x00\x1E\x00"_view);
  LittleBinary reader(aligned_source.get_view());

  EXPECT_EQ(reader.read_bits_16(), Bits_16(0x0A));

  // Read from an incorrect offset which should algin to position 2.
  reader.set_pointer(1);
  EXPECT_EQ(reader.read_bits_16(), Bits_16(0x14));
}

PERIMORTEM_UNIT_TEST(CoreBinaryReader, multiple_readers) {
  using LittleBinary = Reader::Binary<Data::ByteOrder::Little>;
  alignas(alignof(Bits_16)) Static::Bytes<6> aligned_source(
      "\x01\x00\x02\x00\x03\x00"_view);
  View::Bytes source = aligned_source.get_view();
  LittleBinary readers[] = {LittleBinary(source), LittleBinary(source)};

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

PERIMORTEM_UNIT_TEST(CoreBinaryWriter, little_endian_unsigned_integers) {
  using LittleBinary = Writer::Binary<Data::ByteOrder::Little>;
  alignas(alignof(Bits_64)) Static::Bytes<16> buffer;
  LittleBinary writer(buffer.get_access());

  writer << Bits_8(0xAB) << Bits_16(0x1234) << Bits_32(0xDEADBEEF)
         << Bits_64(0x0123456789ABCDEF);

  EXPECT(writer.is_valid());
  EXPECT_EQ(Long(writer.get_location()), Long(16));
  EXPECT_HEX(
      buffer.slice(0, 16),
      "\xAB\x00"                               // Bits_8(0xAB) + pad
      "\x34\x12"                               // Bits_16(0x1234) LE
      "\xEF\xBE\xAD\xDE"                       // Bits_32(0xDEADBEEF) LE
      "\xEF\xCD\xAB\x89\x67\x45\x23\x01"_view  // Bits_64(0x0123456789ABCDEF) LE
  );
}

PERIMORTEM_UNIT_TEST(CoreBinaryWriter, little_endian_signed_integers) {
  using LittleBinary = Writer::Binary<Data::ByteOrder::Little>;
  alignas(alignof(Bits_64)) Static::Bytes<16> buffer;
  LittleBinary writer(buffer.get_access());

  writer << SignedBits_8(-42) << SignedBits_16(-1000) << SignedBits_32(-100000)
         << SignedBits_64(-1000000000LL);

  EXPECT(writer.is_valid());
  EXPECT_HEX(
      buffer.slice(0, 16),
      "\xD6\x00"                               // SignedBits_8(-42) + pad
      "\x18\xFC"                               // SignedBits_16(-1000) LE
      "\x60\x79\xFE\xFF"                       // SignedBits_32(-100000) LE
      "\x00\x36\x65\xC4\xFF\xFF\xFF\xFF"_view  // SignedBits_64(-1000000000) LE
  );
}

PERIMORTEM_UNIT_TEST(CoreBinaryWriter, big_endian_layout) {
  using BigBinary = Writer::Binary<Data::ByteOrder::Big>;
  alignas(alignof(Bits_32)) Static::Bytes<4> buffer;
  BigBinary writer(buffer.get_access());

  writer << Bits_32(0x12345678);

  EXPECT(writer.is_valid());
  EXPECT_HEX(buffer.slice(0, 4), "\x12\x34\x56\x78"_view);
}

PERIMORTEM_UNIT_TEST(CoreBinaryWriter, little_endian_reals) {
  // Real_32(3.0f) = 0x40400000; Real_64(1.5) = 0x3FF8000000000000.
  using LittleBinary = Writer::Binary<Data::ByteOrder::Little>;
  alignas(alignof(Bits_64)) Static::Bytes<16> buffer;
  LittleBinary writer(buffer.get_access());

  writer << Real_32(3.0f) << Real_64(1.5);

  EXPECT(writer.is_valid());
  EXPECT_HEX(
      buffer.slice(0, 16),
      "\x00\x00\x40\x40"                       // Real_32(3.0f) LE
      "\x00\x00\x00\x00"                       // pad to 8-byte align
      "\x00\x00\x00\x00\x00\x00\xF8\x3F"_view  // Real_64(1.5) LE
  );
}

PERIMORTEM_UNIT_TEST(CoreBinaryWriter, raw_bytes) {
  using LittleBinary = Writer::Binary<Data::ByteOrder::Little>;
  Static::Bytes<9> buffer;
  LittleBinary writer(buffer.get_access());

  writer << "blob data"_view;

  EXPECT(writer.is_valid());
  EXPECT_TEXT(buffer.slice(0, 9), "blob data"_view);
}

PERIMORTEM_UNIT_TEST(CoreBinaryWriter, overflow) {
  using LittleBinary = Writer::Binary<Data::ByteOrder::Little>;
  Static::Bytes<3> buffer;
  LittleBinary writer(buffer.get_access());

  writer << Bits_32(0xDEAD);

  EXPECT_NOT(writer.is_valid());
}

PERIMORTEM_UNIT_TEST(CoreBinaryWriter, set_pointer) {
  using LittleBinary = Writer::Binary<Data::ByteOrder::Little>;
  alignas(alignof(Bits_16)) Static::Bytes<6> buffer;
  LittleBinary writer(buffer.get_access());

  writer << Bits_16(0x0A0B) << Bits_16(0x0C0D);
  EXPECT_EQ(Long(writer.get_location()), Long(4));

  writer.set_pointer(0);
  writer << Bits_16(0x1234);

  EXPECT(writer.is_valid());
  EXPECT_HEX(
      buffer.slice(0, 4),
      "\x34\x12"       // overwritten Bits_16(0x1234)
      "\x0D\x0C"_view  // original second value
  );
}

PERIMORTEM_UNIT_TEST(CoreBinaryWriter, multiple_writers) {
  using LittleBinary = Writer::Binary<Data::ByteOrder::Little>;
  alignas(alignof(Bits_16)) Static::Bytes<4> buffer;
  LittleBinary writers[] = {
    LittleBinary(buffer.get_access()),
    LittleBinary(buffer.get_access()),
  };

  writers[0] << Bits_16(0xAAAA);
  writers[1] << Bits_16(0xBBBB);  // overwrites writers[0] at position 0
  writers[0] << Bits_16(0xCCCC);  // writers[0] is now at position 2

  EXPECT(writers[0].is_valid());
  EXPECT(writers[1].is_valid());
  EXPECT_HEX(buffer.slice(0, 4), "\xBB\xBB\xCC\xCC"_view);
}
