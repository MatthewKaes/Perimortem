// Perimortem Engine
// Copyright © Matt Kaes

#include "validation/unit_test.hpp"

#include "perimortem/core/static/bytes.hpp"
#include "perimortem/core/null_terminated.hpp"
#include "perimortem/core/reader/binary.hpp"

using namespace Perimortem::Core;
using namespace Validation;

Test::Harness CoreBinaryReader = {.name = "Core::Reader::Binary"};

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

  auto slice = reader.read_bytes(5);

  EXPECT(reader.is_valid());
  EXPECT_TEXT(slice, "Hello"_view);
  EXPECT_EQ(Long(reader.get_location()), Long(5));
}

PERIMORTEM_UNIT_TEST(CoreBinaryReader, overflow_read) {
  using LittleBinary = Reader::Binary<Data::ByteOrder::Little>;
  LittleBinary reader("\xAB\xCD"_view);

  // Out of bounds read should return null and set the reader to invalid.
  EXPECT_EQ(reader.read_bits_32(), Bits_32(0));
  EXPECT_NOT(reader.is_valid());
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
