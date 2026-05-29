// Perimortem Engine
// Copyright © Matt Kaes

#include "perimortem/serialization/format/png.hpp"

#include "validation/unit_test.hpp"

#include "perimortem/core/static/bytes.hpp"
#include "perimortem/core/algorithm/search.hpp"
#include "perimortem/core/bibliotheca.hpp"
#include "perimortem/core/diagnostics/log.hpp"
#include "perimortem/core/null_terminated.hpp"

#include "perimortem/system/file.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Graphics;
using namespace Perimortem::Memory;
using namespace Perimortem::Serialization;
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

static auto error_contains(View::Bytes message) -> Bool {
  return Algorithm::search(captured_message(), message) != Count(-1);
}

static Harness SerializationPng = {
  .name = "Serialization::Png"_view,
  .setup =
      []() {
        Diagnostics::Log::set_sink(capture_sink);
        captured_log_message = ""_view;
        captured_log_message_size = 0;
      },
  .teardown =
      []() { Diagnostics::Log::set_sink(Diagnostics::Log::default_sink); },
};

PERIMORTEM_UNIT_TEST(SerializationPng, decode_red_1x1_dimensions) {
  auto start_requests = Bibliotheca::check_out_requests();
  File source;
  ASSERT(source.read("validation/data/pngs/red_1x1.png"_view));

  auto image = Format::Png::decode(source.get_view());

  EXPECT_EQ(image.get_width(), Bits_32(1));
  EXPECT_EQ(image.get_height(), Bits_32(1));
  EXPECT_EQ(image.get_color_depth(), Bits_8(8));

  auto pixels = image.get_pixels();
  ASSERT_EQ(pixels.get_size(), Count(1));
  EXPECT_EQ(pixels[0].red, Byte(0xFF));
  EXPECT_EQ(pixels[0].green, Byte(0x00));
  EXPECT_EQ(pixels[0].blue, Byte(0x00));
  EXPECT_EQ(pixels[0].alpha, Byte(0xFF));
  EXPECT(Bibliotheca::check_out_requests() - start_requests <= 6);
}

PERIMORTEM_UNIT_TEST(SerializationPng, decode_checkerboard_2x2) {
  File source;
  ASSERT(source.read("validation/data/pngs/checkerboard_2x2.png"_view));

  auto image = Format::Png::decode(source.get_view());
  auto pixels = image.get_pixels();

  ASSERT_EQ(pixels.get_size(), Count(4));
  EXPECT_EQ(pixels[0].red, Byte(0xFF));
  EXPECT_EQ(pixels[0].green, Byte(0x00));
  EXPECT_EQ(pixels[0].blue, Byte(0x00));
  EXPECT_EQ(pixels[0].alpha, Byte(0xFF));
  EXPECT_EQ(pixels[1].red, Byte(0x00));
  EXPECT_EQ(pixels[1].green, Byte(0xFF));
  EXPECT_EQ(pixels[1].blue, Byte(0x00));
  EXPECT_EQ(pixels[1].alpha, Byte(0xFF));
  EXPECT_EQ(pixels[2].red, Byte(0x00));
  EXPECT_EQ(pixels[2].green, Byte(0x00));
  EXPECT_EQ(pixels[2].blue, Byte(0xFF));
  EXPECT_EQ(pixels[2].alpha, Byte(0xFF));
  EXPECT_EQ(pixels[3].red, Byte(0xFF));
  EXPECT_EQ(pixels[3].green, Byte(0xFF));
  EXPECT_EQ(pixels[3].blue, Byte(0xFF));
  EXPECT_EQ(pixels[3].alpha, Byte(0xFF));
}

PERIMORTEM_UNIT_TEST(SerializationPng, decode_rgb_expands_to_rgba) {
  File source;
  ASSERT(source.read("validation/data/pngs/rgb_3x1.png"_view));

  auto image = Format::Png::decode(source.get_view());
  auto pixels = image.get_pixels();

  // RGB source: alpha must be synthesized as fully opaque.
  ASSERT_EQ(pixels.get_size(), Count(3));
  EXPECT_EQ(pixels[0].red, Byte(0xFF));
  EXPECT_EQ(pixels[0].alpha, Byte(0xFF));
  EXPECT_EQ(pixels[1].green, Byte(0xFF));
  EXPECT_EQ(pixels[1].alpha, Byte(0xFF));
  EXPECT_EQ(pixels[2].blue, Byte(0xFF));
  EXPECT_EQ(pixels[2].alpha, Byte(0xFF));
}

PERIMORTEM_UNIT_TEST(SerializationPng, decode_gray_expands_to_rgba) {
  File source;
  ASSERT(source.read("validation/data/pngs/gray_2x2.png"_view));

  auto image = Format::Png::decode(source.get_view());
  auto pixels = image.get_pixels();

  // Greyscale source: gray value replicates to all three color channels.
  ASSERT_EQ(pixels.get_size(), Count(4));
  EXPECT_EQ(pixels[0].red, Byte(0x00));
  EXPECT_EQ(pixels[0].green, Byte(0x00));
  EXPECT_EQ(pixels[0].blue, Byte(0x00));
  EXPECT_EQ(pixels[0].alpha, Byte(0xFF));

  EXPECT_EQ(pixels[1].red, Byte(0x40));
  EXPECT_EQ(pixels[1].green, Byte(0x40));
  EXPECT_EQ(pixels[1].blue, Byte(0x40));
  EXPECT_EQ(pixels[1].alpha, Byte(0xFF));

  EXPECT_EQ(pixels[2].red, Byte(0x80));
  EXPECT_EQ(pixels[2].green, Byte(0x80));
  EXPECT_EQ(pixels[2].blue, Byte(0x80));
  EXPECT_EQ(pixels[2].alpha, Byte(0xFF));

  EXPECT_EQ(pixels[3].red, Byte(0xFF));
  EXPECT_EQ(pixels[3].green, Byte(0xFF));
  EXPECT_EQ(pixels[3].blue, Byte(0xFF));
  EXPECT_EQ(pixels[3].alpha, Byte(0xFF));
}

PERIMORTEM_UNIT_TEST(SerializationPng, decode_gradient_4x4) {
  File source;
  ASSERT(source.read("validation/data/pngs/gradient_4x4.png"_view));

  auto image = Format::Png::decode(source.get_view());
  auto pixels = image.get_pixels();

  ASSERT_EQ(pixels.get_size(), Count(16));
  EXPECT_EQ(pixels[0].red, Byte(0));
  EXPECT_EQ(pixels[0].green, Byte(0));
  EXPECT_EQ(pixels[0].blue, Byte(128));
  EXPECT_EQ(pixels[15].red, Byte(255));
  EXPECT_EQ(pixels[15].green, Byte(255));
  EXPECT_EQ(pixels[15].blue, Byte(128));
}

PERIMORTEM_UNIT_TEST(SerializationPng, decode_pattern_8x1) {
  File source;
  ASSERT(source.read("validation/data/pngs/pattern_8x1.png"_view));

  auto image = Format::Png::decode(source.get_view());
  auto pixels = image.get_pixels();

  ASSERT_EQ(pixels.get_size(), Count(8));
  EXPECT_EQ(pixels[0].red, Byte(100));
  EXPECT_EQ(pixels[0].green, Byte(200));
  EXPECT_EQ(pixels[0].blue, Byte(50));
  EXPECT_EQ(pixels[0].alpha, Byte(255));
  EXPECT_EQ(pixels[4].red, Byte(100));
  EXPECT_EQ(pixels[4].green, Byte(200));
}

PERIMORTEM_UNIT_TEST(SerializationPng, decode_invalid_returns_empty) {
  const Byte garbage[] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07};
  auto image = Format::Png::decode(View::Bytes(garbage, 8));
  EXPECT_EQ(image.get_width(), Bits_32(0));
  EXPECT_EQ(image.get_height(), Bits_32(0));
}

PERIMORTEM_UNIT_TEST(SerializationPng, encode_decode_roundtrip_1x1) {
  Dynamic::Vector<Pixel> source_pixels;
  source_pixels.insert({0x12, 0x34, 0x56, 0x78});
  Image source_image(1, 1, Data::take(source_pixels));

  auto encoded = Format::Png::encode(source_image);
  ASSERT(encoded.get_size() > 0);

  auto decoded = Format::Png::decode(encoded.get_view());
  EXPECT_EQ(decoded.get_width(), Bits_32(1));
  EXPECT_EQ(decoded.get_height(), Bits_32(1));

  auto decoded_pixels = decoded.get_pixels();
  ASSERT_EQ(decoded_pixels.get_size(), Count(1));
  EXPECT_EQ(decoded_pixels[0].red, Byte(0x12));
  EXPECT_EQ(decoded_pixels[0].green, Byte(0x34));
  EXPECT_EQ(decoded_pixels[0].blue, Byte(0x56));
  EXPECT_EQ(decoded_pixels[0].alpha, Byte(0x78));
}

PERIMORTEM_UNIT_TEST(SerializationPng, encode_decode_roundtrip_checkerboard) {
  Dynamic::Vector<Pixel> source_pixels;
  source_pixels.insert({0xFF, 0x00, 0x00, 0xFF});
  source_pixels.insert({0x00, 0xFF, 0x00, 0xFF});
  source_pixels.insert({0x00, 0x00, 0xFF, 0xFF});
  source_pixels.insert({0xFF, 0xFF, 0xFF, 0xFF});
  Image source_image(2, 2, Data::take(source_pixels));

  auto encoded = Format::Png::encode(source_image);
  ASSERT(encoded.get_size() > 0);

  auto decoded = Format::Png::decode(encoded.get_view());
  EXPECT_EQ(decoded.get_width(), Bits_32(2));
  EXPECT_EQ(decoded.get_height(), Bits_32(2));

  auto decoded_pixels = decoded.get_pixels();
  ASSERT_EQ(decoded_pixels.get_size(), Count(4));
  EXPECT_EQ(decoded_pixels[0].red, Byte(0xFF));
  EXPECT_EQ(decoded_pixels[1].green, Byte(0xFF));
  EXPECT_EQ(decoded_pixels[2].blue, Byte(0xFF));
  EXPECT_EQ(decoded_pixels[3].red, Byte(0xFF));
}

PERIMORTEM_UNIT_TEST(SerializationPng, encode_decode_roundtrip_large) {
  constexpr Count width = 32, height = 32;
  Dynamic::Vector<Pixel> source_pixels;
  source_pixels.resize(width * height);
  for (Count row = 0; row < height; row++) {
    for (Count col = 0; col < width; col++) {
      source_pixels[row * width + col] = {
        Byte(col * 8), Byte(row * 8), Byte(128), Byte(255)};
    }
  }
  Image source_image(
      Bits_32(width), Bits_32(height), Data::take(source_pixels));

  auto encoded = Format::Png::encode(source_image);
  ASSERT(encoded.get_size() > 0);

  auto decoded = Format::Png::decode(encoded.get_view());
  auto source_view = source_image.get_pixels();
  auto decoded_pixels = decoded.get_pixels();
  ASSERT_EQ(decoded_pixels.get_size(), Count(width * height));
  for (Count pixel_index = 0; pixel_index < width * height; pixel_index++) {
    EXPECT_EQ(decoded_pixels[pixel_index].red, source_view[pixel_index].red);
    EXPECT_EQ(
        decoded_pixels[pixel_index].green, source_view[pixel_index].green);
    EXPECT_EQ(decoded_pixels[pixel_index].blue, source_view[pixel_index].blue);
    EXPECT_EQ(
        decoded_pixels[pixel_index].alpha, source_view[pixel_index].alpha);
  }
}

PERIMORTEM_UNIT_TEST(SerializationPng, encode_wrong_pixel_count_returns_empty) {
  Dynamic::Vector<Pixel> one_pixel;
  one_pixel.insert({0xFF, 0x00, 0x00, 0xFF});
  Image bad_image(2, 2, Data::take(one_pixel));
  auto encoded = Format::Png::encode(bad_image);
  EXPECT_EQ(encoded.get_size(), Count(0));
}

PERIMORTEM_UNIT_TEST(SerializationPng, encode_zero_dimensions_returns_empty) {
  Dynamic::Vector<Pixel> empty;
  Image zero_width(0, 1, Data::take(empty));
  Dynamic::Vector<Pixel> empty2;
  Image zero_height(1, 0, Data::take(empty2));
  auto encoded_w = Format::Png::encode(zero_width);
  auto encoded_h = Format::Png::encode(zero_height);
  EXPECT_EQ(encoded_w.get_size(), Count(0));
  EXPECT_EQ(encoded_h.get_size(), Count(0));
}

PERIMORTEM_UNIT_TEST(SerializationPng, truncated_chunk_header) {
  // PNG with a valid IHDR but only 4 bytes of a second chunk — not enough
  // for the 12-byte chunk header overhead, so read_chunk fires an error.
  File source;
  ASSERT(source.read("validation/data/pngs/error_truncated_header.png"_view));

  auto image = Format::Png::decode(source.get_view());

  EXPECT_EQ(image.get_width(), Bits_32(0));
  EXPECT_EQ(UInt(captured_log_level), UInt(Diagnostics::Log::Level::Error));
  EXPECT(error_contains("Png: chunk at offset 33"_view));
  EXPECT(error_contains("truncated before header end"_view));
}

PERIMORTEM_UNIT_TEST(SerializationPng, chunk_length_overrun) {
  // PNG whose second chunk claims a length of 4294967295 bytes — far more
  // data than the file contains, so read_chunk fires an error.
  File source;
  ASSERT(source.read("validation/data/pngs/error_overrun.png"_view));

  auto image = Format::Png::decode(source.get_view());

  EXPECT_EQ(image.get_width(), Bits_32(0));
  EXPECT_EQ(UInt(captured_log_level), UInt(Diagnostics::Log::Level::Error));
  EXPECT(error_contains("Png: chunk at offset 33"_view));
  EXPECT(error_contains("100"_view));
  EXPECT(error_contains("extends past end of stream"_view));
}

PERIMORTEM_UNIT_TEST(SerializationPng, crc_mismatch_names_chunk) {
  // PNG whose IHDR CRC has been corrupted — read_chunk should log the chunk
  // type and offset so the caller can identify which chunk was damaged.
  File source;
  ASSERT(source.read("validation/data/pngs/error_crc_mismatch.png"_view));

  auto image = Format::Png::decode(source.get_view());

  EXPECT_EQ(image.get_width(), Bits_32(0));
  EXPECT_EQ(UInt(captured_log_level), UInt(Diagnostics::Log::Level::Error));
  EXPECT(error_contains("CRC-32 mismatch"_view));
  EXPECT(error_contains("IHDR"_view));
  EXPECT(error_contains("offset 8"_view));
}
