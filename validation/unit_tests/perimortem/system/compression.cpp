// Perimortem Engine
// Copyright © Matt Kaes

#include "perimortem/system/compression.hpp"

#include "validation/unit_test.hpp"

#include "perimortem/core/bibliotheca.hpp"
#include "perimortem/core/null_terminated.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Perimortem::System;

using namespace Validation;

static Harness SystemCompression = {
  .name = "System::Compression"_view,
};

// Known test blobs produced by zlib

// "Hello, Perimortem!"
static const Byte hello_compressed[] = {
  0x78, 0xDA, 0xF3, 0x48, 0xCD, 0xC9, 0xC9, 0xD7, 0x51, 0x08, 0x48, 0x2D, 0xCA,
  0xCC, 0xCD, 0x2F, 0x2A, 0x49, 0xCD, 0x55, 0x04, 0x00, 0x3D, 0x2E, 0x06, 0x86};
static const Byte hello_raw[] = {0x48, 0x65, 0x6C, 0x6C, 0x6F, 0x2C,
                                 0x20, 0x50, 0x65, 0x72, 0x69, 0x6D,
                                 0x6F, 0x72, 0x74, 0x65, 0x6D, 0x21};

// "Stored block test data ABCDEFGHIJ" — level 0
static const Byte stored_compressed[] = {
  0x78, 0x01, 0x01, 0x21, 0x00, 0xDE, 0xFF, 0x53, 0x74, 0x6F, 0x72,
  0x65, 0x64, 0x20, 0x62, 0x6C, 0x6F, 0x63, 0x6B, 0x20, 0x74, 0x65,
  0x73, 0x74, 0x20, 0x64, 0x61, 0x74, 0x61, 0x20, 0x41, 0x42, 0x43,
  0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4A, 0xC9, 0x70, 0x0B, 0x0E};
static const Byte stored_raw[] = {
  0x53, 0x74, 0x6F, 0x72, 0x65, 0x64, 0x20, 0x62, 0x6C, 0x6F, 0x63,
  0x6B, 0x20, 0x74, 0x65, 0x73, 0x74, 0x20, 0x64, 0x61, 0x74, 0x61,
  0x20, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4A};

// 0xAB 0xCD repeated 50 times
static const Byte repeat_compressed[] = {
  0x78, 0xDA, 0x5B, 0x7D, 0x76, 0x35, 0xCD, 0x21, 0x00, 0x7A, 0x7C, 0x49, 0x71};

// Empty input
static const Byte empty_compressed[] = {0x78, 0xDA, 0x03, 0x00,
                                        0x00, 0x00, 0x00, 0x01};

// Single byte 0x42
static const Byte single_compressed[] = {0x78, 0xDA, 0x73, 0x02, 0x00,
                                         0x00, 0x43, 0x00, 0x43};

// hello_compressed with the last byte of its checksum flipped
static const Byte bad_adler_compressed[] = {
  0x78, 0xDA, 0xF3, 0x48, 0xCD, 0xC9, 0xC9, 0xD7, 0x51, 0x08, 0x48, 0x2D, 0xCA,
  0xCC, 0xCD, 0x2F, 0x2A, 0x49, 0xCD, 0x55, 0x04, 0x00, 0x3D, 0x2E, 0x06, 0x79};

PERIMORTEM_UNIT_TEST(SystemCompression, inflate_dynamic_huffman) {
  auto start = Bibliotheca::check_out_requests();
  auto out = Compression::inflate(
      View::Bytes(hello_compressed, sizeof(hello_compressed)));

  ASSERT_EQ(out.get_size(), Count(sizeof(hello_raw)));
  EXPECT_HEX(out.get_view(), View::Bytes(hello_raw, sizeof(hello_raw)));

  // One allocation for the output buffer.
  EXPECT_EQ(Bibliotheca::check_out_requests(), start + 1);
}

PERIMORTEM_UNIT_TEST(SystemCompression, inflate_stored_blocks) {
  auto out = Compression::inflate(
      View::Bytes(stored_compressed, sizeof(stored_compressed)));

  ASSERT_EQ(out.get_size(), Count(sizeof(stored_raw)));
  EXPECT_HEX(out.get_view(), View::Bytes(stored_raw, sizeof(stored_raw)));
}

PERIMORTEM_UNIT_TEST(SystemCompression, inflate_back_references) {
  auto out = Compression::inflate(
      View::Bytes(repeat_compressed, sizeof(repeat_compressed)));

  // Check that ABCD bytes are repeated 50 times.
  ASSERT_EQ(out.get_size(), Count(100));
  for (Count i = 0; i < 100; i += 2) {
    EXPECT_EQ(out[i + 0], Byte(0xAB));
    EXPECT_EQ(out[i + 1], Byte(0xCD));
  }
}

PERIMORTEM_UNIT_TEST(SystemCompression, inflate_empty_stream) {
  auto out = Compression::inflate(
      View::Bytes(empty_compressed, sizeof(empty_compressed)));
  EXPECT_EQ(out.get_size(), Count(0));
}

PERIMORTEM_UNIT_TEST(SystemCompression, inflate_single_byte) {
  auto out = Compression::inflate(
      View::Bytes(single_compressed, sizeof(single_compressed)));

  ASSERT_EQ(out.get_size(), Count(1));
  EXPECT_EQ(out[0], Byte(0x42));
}

PERIMORTEM_UNIT_TEST(SystemCompression, inflate_empty_input_returns_empty) {
  auto out = Compression::inflate(""_view);
  EXPECT_EQ(out.get_size(), Count(0));
}

PERIMORTEM_UNIT_TEST(SystemCompression, inflate_truncated_input_returns_empty) {
  // Hand the decompressor only the zlib header — the deflate payload is
  // missing.
  auto out = Compression::inflate(View::Bytes(hello_compressed, 2));
  EXPECT_EQ(out.get_size(), Count(0));
}

PERIMORTEM_UNIT_TEST(
    SystemCompression,
    inflate_bad_compression_method_returns_empty) {
  Byte bad_cm[sizeof(hello_compressed)];
  Data::copy(bad_cm, hello_compressed, Count(sizeof(hello_compressed)));

  // Corrupt the CM
  bad_cm[0] = (bad_cm[0] & 0xF0) | 0x09;

  auto out = Compression::inflate(View::Bytes(bad_cm, sizeof(bad_cm)));
  EXPECT_EQ(out.get_size(), Count(0));
}

PERIMORTEM_UNIT_TEST(SystemCompression, inflate_bad_checksum_returns_empty) {
  auto out = Compression::inflate(
      View::Bytes(bad_adler_compressed, sizeof(bad_adler_compressed)));
  EXPECT_EQ(out.get_size(), Count(0));
}

PERIMORTEM_UNIT_TEST(
    SystemCompression,
    inflate_corrupted_payload_returns_empty) {
  // Flip a byte in the middle of the DEFLATE bitstream.
  Byte corrupt[sizeof(hello_compressed)];
  Data::copy(corrupt, hello_compressed, Count(sizeof(hello_compressed)));
  corrupt[5] ^= 0xFF;
  auto out = Compression::inflate(View::Bytes(corrupt, sizeof(corrupt)));
  EXPECT_EQ(out.get_size(), Count(0));
}

PERIMORTEM_UNIT_TEST(SystemCompression, deflate_empty_input) {
  auto start = Bibliotheca::check_out_requests();
  auto compressed = Compression::deflate(""_view);

  // A valid zlib stream must still have a header and a footer.
  EXPECT(compressed.get_size() >= 6);

  // Round tripping inflate should properly produce an emtpy buffer.
  auto recovered = Compression::inflate(compressed.get_view());
  EXPECT_EQ(recovered.get_size(), Count(0));

  // 3 allocations in total expected
  // deflate constructor + forgetful_resize (deflate always pre-sizes its output
  // buffer in two steps) + inflate's ensure_capacity.
  EXPECT_EQ(Bibliotheca::check_out_requests(), start + 2);
}

PERIMORTEM_UNIT_TEST(SystemCompression, deflate_single_byte) {
  const Byte src[] = {0x42};
  auto compressed = Compression::deflate(View::Bytes(src, 1));
  EXPECT(compressed.get_size() > 0);

  auto recovered = Compression::inflate(compressed.get_view());
  ASSERT_EQ(recovered.get_size(), Count(1));
  EXPECT_EQ(recovered[0], Byte(0x42));
}

PERIMORTEM_UNIT_TEST(SystemCompression, deflate_roundtrip_short) {
  auto compressed =
      Compression::deflate(View::Bytes(hello_raw, sizeof(hello_raw)));
  ASSERT(compressed.get_size() > 0);

  auto recovered = Compression::inflate(compressed.get_view());
  ASSERT_EQ(recovered.get_size(), Count(sizeof(hello_raw)));
  EXPECT_HEX(recovered.get_view(), View::Bytes(hello_raw, sizeof(hello_raw)));
}

PERIMORTEM_UNIT_TEST(SystemCompression, deflate_roundtrip_binary) {
  // Binary data with all 256 byte values present.
  Byte all_bytes[256];
  for (Count i = 0; i < 256; i++) {
    all_bytes[i] = Byte(i);
  }
  auto compressed = Compression::deflate(View::Bytes(all_bytes, 256));
  ASSERT(compressed.get_size() > 0);

  auto recovered = Compression::inflate(compressed.get_view());
  ASSERT_EQ(recovered.get_size(), Count(256));
  EXPECT_HEX(recovered.get_view(), View::Bytes(all_bytes, 256));
}

PERIMORTEM_UNIT_TEST(SystemCompression, deflate_output_is_valid_zlib) {
  auto compressed =
      Compression::deflate(View::Bytes(stored_raw, sizeof(stored_raw)));

  // Valid zlib: CMF byte must have CM==8.
  ASSERT(compressed.get_size() >= 6);
  EXPECT_EQ(Bits_8(compressed[0] & 0x0F), Bits_8(8));
  // (CMF*256 + FLG) % 31 == 0 is the zlib fcheck constraint.
  EXPECT_EQ((Bits_32(compressed[0]) * 256 + compressed[1]) % 31, Bits_32(0));
}

PERIMORTEM_UNIT_TEST(SystemCompression, deflate_inflate_roundtrip_large) {
  // 8 KB of structured data spanning multiple stored blocks in deflate output.
  constexpr Count size = 8192;
  Byte large[size];
  for (Count i = 0; i < size; i++) {
    large[i] = Byte((i * 31 + i / 128) & 0xFF);
  }

  auto compressed = Compression::deflate(View::Bytes(large, size));
  ASSERT(compressed.get_size() > 0);

  auto recovered = Compression::inflate(compressed.get_view());
  ASSERT_EQ(recovered.get_size(), size);
  EXPECT_HEX(recovered.get_view(), View::Bytes(large, size));
}
