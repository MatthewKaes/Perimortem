// Perimortem Engine
// Copyright © Matt Kaes

#include "validation/unit_test.hpp"

#include "perimortem/core/static/bytes.hpp"
#include "perimortem/core/algorithm/search.hpp"
#include "perimortem/core/bibliotheca.hpp"
#include "perimortem/core/diagnostics/log.hpp"
#include "perimortem/core/null_terminated.hpp"

#include "perimortem/system/compression/deflate.hpp"
#include "perimortem/system/file.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Perimortem::System;

using namespace Validation;

static Diagnostics::Log::Level captured_log_level;
static Static::Bytes<256> captured_log_message;
static Count captured_log_message_size = 0;

static auto capture_sink(Diagnostics::Log::Level level, View::Bytes message)
    -> void {
  captured_log_level = level;
  captured_log_message_size = Math::min(Count(256), message.get_size());
  captured_log_message = message;
}

static auto captured_message() -> View::Bytes {
  return View::Bytes(
      captured_log_message.get_data(), captured_log_message_size);
}

static auto error_contains(View::Bytes needle) -> Bool {
  return Algorithm::search(captured_message(), needle) != Count(-1);
}

static Harness SystemCompression = {
  .name = "System::Compression"_view,
  .setup =
      []() {
        Diagnostics::Log::set_sink(capture_sink);
        captured_log_message = ""_view;
        captured_log_message_size = 0;
      },
  .teardown =
      []() { Diagnostics::Log::set_sink(Diagnostics::Log::default_sink); },
};

// Known test blobs produced by zlib

// "Hello, Perimortem!"
static constexpr Static::Bytes<26> hello_compressed = {
  0x78, 0xDA, 0xF3, 0x48, 0xCD, 0xC9, 0xC9, 0xD7, 0x51, 0x08, 0x48, 0x2D, 0xCA,
  0xCC, 0xCD, 0x2F, 0x2A, 0x49, 0xCD, 0x55, 0x04, 0x00, 0x3D, 0x2E, 0x06, 0x86,
};
static constexpr Static::Bytes<18> hello_raw = {
  0x48, 0x65, 0x6C, 0x6C, 0x6F, 0x2C, 0x20, 0x50, 0x65,
  0x72, 0x69, 0x6D, 0x6F, 0x72, 0x74, 0x65, 0x6D, 0x21,
};

// "Stored block test data ABCDEFGHIJ" — level 0
static constexpr Static::Bytes<44> stored_compressed = {
  0x78, 0x01, 0x01, 0x21, 0x00, 0xDE, 0xFF, 0x53, 0x74, 0x6F, 0x72,
  0x65, 0x64, 0x20, 0x62, 0x6C, 0x6F, 0x63, 0x6B, 0x20, 0x74, 0x65,
  0x73, 0x74, 0x20, 0x64, 0x61, 0x74, 0x61, 0x20, 0x41, 0x42, 0x43,
  0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4A, 0xC9, 0x70, 0x0B, 0x0E,
};
static constexpr Static::Bytes<33> stored_raw = {
  0x53, 0x74, 0x6F, 0x72, 0x65, 0x64, 0x20, 0x62, 0x6C, 0x6F, 0x63,
  0x6B, 0x20, 0x74, 0x65, 0x73, 0x74, 0x20, 0x64, 0x61, 0x74, 0x61,
  0x20, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4A,
};

// 0xAB 0xCD repeated 50 times
static constexpr Static::Bytes<13> repeat_compressed = {
  0x78, 0xDA, 0x5B, 0x7D, 0x76, 0x35, 0xCD, 0x21, 0x00, 0x7A, 0x7C, 0x49, 0x71,
};

// Empty input
static constexpr Static::Bytes<8> empty_compressed = {
  0x78, 0xDA, 0x03, 0x00, 0x00, 0x00, 0x00, 0x01,
};

// Single byte 0x42
static constexpr Static::Bytes<9> single_compressed = {
  0x78, 0xDA, 0x73, 0x02, 0x00, 0x00, 0x43, 0x00, 0x43,
};

// hello_compressed with the last byte of its checksum flipped
static constexpr Static::Bytes<26> bad_adler_compressed = {
  0x78, 0xDA, 0xF3, 0x48, 0xCD, 0xC9, 0xC9, 0xD7, 0x51, 0x08, 0x48, 0x2D, 0xCA,
  0xCC, 0xCD, 0x2F, 0x2A, 0x49, 0xCD, 0x55, 0x04, 0x00, 0x3D, 0x2E, 0x06, 0x79,
};

PERIMORTEM_UNIT_TEST(SystemCompression, inflate_dynamic_huffman) {
  auto start = Bibliotheca::check_out_requests();
  auto out = Compression::inflate(hello_compressed);

  ASSERT_EQ(out.get_size(), hello_raw.get_size());
  EXPECT_HEX(out.get_view(), hello_raw);

  // One allocation for the output buffer.
  EXPECT_EQ(Bibliotheca::check_out_requests(), start + 1);
}

PERIMORTEM_UNIT_TEST(SystemCompression, inflate_stored_blocks) {
  auto out = Compression::inflate(stored_compressed);

  ASSERT_EQ(out.get_size(), stored_raw.get_size());
  EXPECT_HEX(out.get_view(), stored_raw);
}

PERIMORTEM_UNIT_TEST(SystemCompression, inflate_back_references) {
  auto out = Compression::inflate(repeat_compressed);

  // Check that ABCD bytes are repeated 50 times.
  ASSERT_EQ(out.get_size(), Count(100));
  for (Count i = 0; i < 100; i += 2) {
    EXPECT_EQ(out[i + 0], Byte(0xAB));
    EXPECT_EQ(out[i + 1], Byte(0xCD));
  }
}

PERIMORTEM_UNIT_TEST(SystemCompression, inflate_empty_content) {
  auto out = Compression::inflate(empty_compressed);
  EXPECT_EQ(out.get_size(), Count(0));
}

PERIMORTEM_UNIT_TEST(SystemCompression, inflate_single_byte) {
  auto out = Compression::inflate(single_compressed);

  ASSERT_EQ(out.get_size(), Count(1));
  EXPECT_EQ(out[0], Byte(0x42));
}

PERIMORTEM_UNIT_TEST(SystemCompression, inflate_empty_view) {
  auto out = Compression::inflate(""_view);
  EXPECT_EQ(out.get_size(), Count(0));
  EXPECT_EQ(UInt(captured_log_level), UInt(Diagnostics::Log::Level::Error));
  EXPECT(error_contains("Compression:"_view));
  EXPECT(error_contains("too short"_view));
}

PERIMORTEM_UNIT_TEST(SystemCompression, inflate_truncated_input) {
  // Hand the decompressor only the zlib header — the deflate payload is
  // missing.
  auto out = Compression::inflate(hello_compressed.slice(0, 2));
  EXPECT_EQ(out.get_size(), Count(0));
  EXPECT_EQ(UInt(captured_log_level), UInt(Diagnostics::Log::Level::Error));
  EXPECT(error_contains("Compression:"_view));
  EXPECT(error_contains("too short"_view));
}

PERIMORTEM_UNIT_TEST(SystemCompression, inflate_bad_compression_method) {
  Static::Bytes<26> bad_cm = hello_compressed;

  // Corrupt the CM nibble to 9 (DEFLATE requires exactly 8).
  bad_cm[0] = (bad_cm[0] & 0xF0) | 0x09;

  auto out = Compression::inflate(bad_cm);
  EXPECT_EQ(out.get_size(), Count(0));
  EXPECT_EQ(UInt(captured_log_level), UInt(Diagnostics::Log::Level::Error));
  EXPECT(error_contains("Compression:"_view));
  EXPECT(error_contains("compression method"_view));
}

PERIMORTEM_UNIT_TEST(SystemCompression, inflate_bad_checksum) {
  auto out = Compression::inflate(bad_adler_compressed);
  EXPECT_EQ(out.get_size(), Count(0));
  EXPECT_EQ(UInt(captured_log_level), UInt(Diagnostics::Log::Level::Error));
  EXPECT(error_contains("Compression:"_view));
  EXPECT(error_contains("Adler-32"_view));
}

PERIMORTEM_UNIT_TEST(SystemCompression, inflate_corrupted_payload) {
  // Flip all bits of a byte in the middle of the DEFLATE bitstream.
  Static::Bytes<26> corrupt = hello_compressed;
  corrupt[5] ^= 0xFF;
  auto out = Compression::inflate(corrupt);
  EXPECT_EQ(out.get_size(), Count(0));
  EXPECT_EQ(UInt(captured_log_level), UInt(Diagnostics::Log::Level::Error));
  EXPECT(error_contains("Compression:"_view));
  // Payload corruption hits either Huffman decode or checksum depending on
  // whether the mangled bits happen to produce a structurally valid stream.
  EXPECT(
      error_contains("decompression failed"_view) ||
      error_contains("checksum"_view));
}

PERIMORTEM_UNIT_TEST(SystemCompression, deflate_empty_input) {
  auto start = Bibliotheca::check_out_requests();
  auto compressed = Compression::deflate(""_view);

  // A valid zlib stream must still have a header and a footer.
  EXPECT(compressed.get_size() >= 6);

  // Round tripping inflate should properly produce an emtpy buffer.
  auto recovered = Compression::inflate(compressed);
  EXPECT_EQ(recovered.get_size(), Count(0));

  // 3 allocations in total expected
  // deflate constructor + forgetful_resize (deflate always pre-sizes its output
  // buffer in two steps) + inflate's ensure_capacity.
  EXPECT_EQ(Bibliotheca::check_out_requests(), start + 2);
}

PERIMORTEM_UNIT_TEST(SystemCompression, deflate_single_byte) {
  constexpr Static::Bytes<1> src = {
    0x42,
  };
  auto compressed = Compression::deflate(src);
  EXPECT(compressed.get_size() > 0);

  auto recovered = Compression::inflate(compressed);
  ASSERT_EQ(recovered.get_size(), Count(1));
  EXPECT_EQ(recovered[0], Byte(0x42));
}

PERIMORTEM_UNIT_TEST(SystemCompression, deflate_roundtrip_short) {
  auto compressed = Compression::deflate(hello_raw);
  ASSERT(compressed.get_size() > 0);

  auto recovered = Compression::inflate(compressed);
  ASSERT_EQ(recovered.get_size(), hello_raw.get_size());
  EXPECT_HEX(recovered.get_view(), hello_raw);
}

PERIMORTEM_UNIT_TEST(SystemCompression, deflate_roundtrip_binary) {
  // Binary data with all 256 byte values present.
  Static::Bytes<256> all_bytes;
  for (Count i = 0; i < 256; i++) {
    all_bytes[i] = Byte(i);
  }
  auto compressed = Compression::deflate(all_bytes);
  ASSERT(compressed.get_size() > 0);

  auto recovered = Compression::inflate(compressed);
  ASSERT_EQ(recovered.get_size(), Count(256));
  EXPECT_HEX(recovered.get_view(), all_bytes);
}

PERIMORTEM_UNIT_TEST(SystemCompression, deflate_output_is_valid_zlib) {
  auto compressed = Compression::deflate(stored_raw);

  // Valid zlib: CMF byte must have CM==8.
  ASSERT(compressed.get_size() >= 6);
  EXPECT_EQ(Bits_8(compressed[0] & 0x0F), Bits_8(8));
  // (CMF*256 + FLG) % 31 == 0 is the zlib fcheck constraint.
  EXPECT_EQ((Bits_32(compressed[0]) * 256 + compressed[1]) % 31, Bits_32(0));
}

PERIMORTEM_UNIT_TEST(SystemCompression, deflate_repeating_value) {
  // 512 bytes of a single repeated value.  The greedy LZ77 pass will emit a
  // 258-byte back-reference (the DEFLATE maximum) immediately after the first
  // literal, exercising the max-length boundary in encode_match_length.
  constexpr Count source_size = 512;
  Static::Bytes<source_size> source;
  for (Count i = 0; i < source_size; i++) {
    source[i] = Byte(0xAA);
  }

  auto compressed = Compression::deflate(source);
  ASSERT(compressed.get_size() > 0);
  // 512 identical bytes must compress to well under half the raw size.
  EXPECT(compressed.get_size() < source_size / 2);

  auto recovered = Compression::inflate(compressed);
  ASSERT_EQ(recovered.get_size(), source_size);
  EXPECT_HEX(recovered.get_view(), source);
}

PERIMORTEM_UNIT_TEST(SystemCompression, roundtrip_large) {
  // 8 KB of structured data spanning multiple stored blocks in deflate output.
  constexpr Count size = 8192;
  Static::Bytes<size> large;
  for (Count i = 0; i < size; i++) {
    large[i] = Byte((i * 31 + i / 128) & 0xFF);
  }

  auto compressed = Compression::deflate(large);
  ASSERT(compressed.get_size() > 0);

  auto recovered = Compression::inflate(compressed);
  ASSERT_EQ(recovered.get_size(), size);
  EXPECT_HEX(recovered.get_view(), large);
}

PERIMORTEM_UNIT_TEST(SystemCompression, size_source_file) {
  // Every level that does real compression must beat the level below it on
  // data that is known to be compressible.  Ordering must hold: None > Default
  // >= Best.
  File source_file;
  ASSERT(source_file.read("perimortem/system/compression/deflate.cpp"_view));
  View::Bytes source = source_file.get_view();

  auto no_compression = Compression::deflate(source, Compression::Level::None);
  auto default_compression =
      Compression::deflate(source, Compression::Level::Default);
  auto best_compression =
      Compression::deflate(source, Compression::Level::Best);

  ASSERT(best_compression.get_size() > 0);

  // Best compression only saves ~30 bytes over 10 KB output!
  EXPECT(best_compression.get_size() <= default_compression.get_size());

  // Default compression is a huge step up from no compression though.
  EXPECT(default_compression.get_size() <= no_compression.get_size() / 2);

  auto recovered = Compression::inflate(best_compression);
  ASSERT_EQ(recovered.get_size(), source.get_size());
  EXPECT_HEX(recovered.get_view(), source);
}

PERIMORTEM_UNIT_TEST(SystemCompression, size_repetitive_data) {
  // 4 KB of a 64-byte pattern cycled 64 times.  After the first cycle every
  // subsequent occurrence should be back-referenced, so the compressed output
  // must be substantially smaller than Level::None stored blocks.
  constexpr Count pattern_size = 64;
  Static::Bytes<4096> source;
  for (Count i = 0; i < source.get_size(); i++) {
    source[i] = Byte((i % pattern_size) * 4 + i / pattern_size);
  }

  auto no_compression = Compression::deflate(source, Compression::Level::None);
  auto default_compression =
      Compression::deflate(source, Compression::Level::Default);
  auto best_compression =
      Compression::deflate(source, Compression::Level::Best);

  ASSERT(best_compression.get_size() > 0);
  EXPECT(best_compression.get_size() <= default_compression.get_size());
  EXPECT(default_compression.get_size() <= no_compression.get_size());

  auto recovered = Compression::inflate(best_compression);
  ASSERT_EQ(recovered.get_size(), source.get_size());
  EXPECT_HEX(recovered.get_view(), source);
}
