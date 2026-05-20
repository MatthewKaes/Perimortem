// Perimortem Engine
// Copyright © Matt Kaes

#include "validation/unit_test.hpp"

#include "perimortem/core/static/bytes.hpp"
#include "perimortem/core/null_terminated.hpp"
#include "perimortem/core/writer/binary.hpp"

using namespace Perimortem::Core;
using namespace Validation;

Test::Harness CoreBinaryWriter = {.name = "Core::Writer::Binary"};

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
