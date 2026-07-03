// Perimortem Engine
// Copyright © Matt Kaes

#include "perimortem/serialization/format/png.hpp"

#include "validation/unit_test.hpp"

#include "perimortem/core/static/bytes.hpp"
#include "perimortem/core/algorithm/search.hpp"
#include "perimortem/core/bibliotheca.hpp"
#include "perimortem/core/null_terminated.hpp"

#include "perimortem/system/file.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Graphics;
using namespace Perimortem::Memory;
using namespace Perimortem::Serialization;
using namespace Perimortem::System;
using namespace Validation;

static Harness SerializationPng = {
  .name = "Serialization::Png"_view,
};

PERIMORTEM_UNIT_TEST(SerializationPng, red_1x1_dimensions) {
  auto start_requests = Bibliotheca::check_out_requests();
  File source;
  ASSERT(source.read("validation/data/pngs/red_1x1.png"_view));

  auto image = Format::Png::decode(source.get_view());

  EXPECT_EQ(image.get_width(), Bits_32(1));
  EXPECT_EQ(image.get_height(), Bits_32(1));
  EXPECT_EQ(image.get_color_depth(), Bits_8(8));

  auto pixels = image.get_pixels();
  ASSERT_EQ(pixels.get_size(), Count(1));
  EXPECT_EQ(pixels[0].red, Bits_8(0xFF));
  EXPECT_EQ(pixels[0].green, Bits_8(0x00));
  EXPECT_EQ(pixels[0].blue, Bits_8(0x00));
  EXPECT_EQ(pixels[0].alpha, Bits_8(0xFF));
  EXPECT(Bibliotheca::check_out_requests() - start_requests <= 6);
}

PERIMORTEM_UNIT_TEST(SerializationPng, checkerboard_2x2) {
  File source;
  ASSERT(source.read("validation/data/pngs/checkerboard_2x2.png"_view));

  auto image = Format::Png::decode(source.get_view());
  auto pixels = image.get_pixels();

  ASSERT_EQ(pixels.get_size(), Count(4));
  EXPECT_EQ(pixels[0].red, Bits_8(0xFF));
  EXPECT_EQ(pixels[0].green, Bits_8(0x00));
  EXPECT_EQ(pixels[0].blue, Bits_8(0x00));
  EXPECT_EQ(pixels[0].alpha, Bits_8(0xFF));
  EXPECT_EQ(pixels[1].red, Bits_8(0x00));
  EXPECT_EQ(pixels[1].green, Bits_8(0xFF));
  EXPECT_EQ(pixels[1].blue, Bits_8(0x00));
  EXPECT_EQ(pixels[1].alpha, Bits_8(0xFF));
  EXPECT_EQ(pixels[2].red, Bits_8(0x00));
  EXPECT_EQ(pixels[2].green, Bits_8(0x00));
  EXPECT_EQ(pixels[2].blue, Bits_8(0xFF));
  EXPECT_EQ(pixels[2].alpha, Bits_8(0xFF));
  EXPECT_EQ(pixels[3].red, Bits_8(0xFF));
  EXPECT_EQ(pixels[3].green, Bits_8(0xFF));
  EXPECT_EQ(pixels[3].blue, Bits_8(0xFF));
  EXPECT_EQ(pixels[3].alpha, Bits_8(0xFF));
}

PERIMORTEM_UNIT_TEST(SerializationPng, decode_rgb_to_rgba) {
  File source;
  ASSERT(source.read("validation/data/pngs/rgb_3x1.png"_view));

  auto image = Format::Png::decode(source.get_view());
  auto pixels = image.get_pixels();

  // RGB source: alpha must be synthesized as fully opaque.
  ASSERT_EQ(pixels.get_size(), Count(3));
  EXPECT_EQ(pixels[0].red, Bits_8(0xFF));
  EXPECT_EQ(pixels[0].alpha, Bits_8(0xFF));
  EXPECT_EQ(pixels[1].green, Bits_8(0xFF));
  EXPECT_EQ(pixels[1].alpha, Bits_8(0xFF));
  EXPECT_EQ(pixels[2].blue, Bits_8(0xFF));
  EXPECT_EQ(pixels[2].alpha, Bits_8(0xFF));
}

PERIMORTEM_UNIT_TEST(SerializationPng, gray_to_rgba) {
  File source;
  ASSERT(source.read("validation/data/pngs/gray_2x2.png"_view));

  auto image = Format::Png::decode(source.get_view());
  auto pixels = image.get_pixels();

  // Greyscale source: gray value replicates to all three color channels.
  ASSERT_EQ(pixels.get_size(), Count(4));
  EXPECT_EQ(pixels[0].red, Bits_8(0x00));
  EXPECT_EQ(pixels[0].green, Bits_8(0x00));
  EXPECT_EQ(pixels[0].blue, Bits_8(0x00));
  EXPECT_EQ(pixels[0].alpha, Bits_8(0xFF));

  EXPECT_EQ(pixels[1].red, Bits_8(0x40));
  EXPECT_EQ(pixels[1].green, Bits_8(0x40));
  EXPECT_EQ(pixels[1].blue, Bits_8(0x40));
  EXPECT_EQ(pixels[1].alpha, Bits_8(0xFF));

  EXPECT_EQ(pixels[2].red, Bits_8(0x80));
  EXPECT_EQ(pixels[2].green, Bits_8(0x80));
  EXPECT_EQ(pixels[2].blue, Bits_8(0x80));
  EXPECT_EQ(pixels[2].alpha, Bits_8(0xFF));

  EXPECT_EQ(pixels[3].red, Bits_8(0xFF));
  EXPECT_EQ(pixels[3].green, Bits_8(0xFF));
  EXPECT_EQ(pixels[3].blue, Bits_8(0xFF));
  EXPECT_EQ(pixels[3].alpha, Bits_8(0xFF));
}

PERIMORTEM_UNIT_TEST(SerializationPng, decode_gradient_4x4) {
  File source;
  ASSERT(source.read("validation/data/pngs/gradient_4x4.png"_view));

  auto image = Format::Png::decode(source.get_view());
  auto pixels = image.get_pixels();

  ASSERT_EQ(pixels.get_size(), Count(16));
  EXPECT_EQ(pixels[0].red, Bits_8(0));
  EXPECT_EQ(pixels[0].green, Bits_8(0));
  EXPECT_EQ(pixels[0].blue, Bits_8(128));
  EXPECT_EQ(pixels[15].red, Bits_8(255));
  EXPECT_EQ(pixels[15].green, Bits_8(255));
  EXPECT_EQ(pixels[15].blue, Bits_8(128));
}

PERIMORTEM_UNIT_TEST(SerializationPng, decode_pattern_8x1) {
  File source;
  ASSERT(source.read("validation/data/pngs/pattern_8x1.png"_view));

  auto image = Format::Png::decode(source.get_view());
  auto pixels = image.get_pixels();

  ASSERT_EQ(pixels.get_size(), Count(8));
  EXPECT_EQ(pixels[0].red, Bits_8(100));
  EXPECT_EQ(pixels[0].green, Bits_8(200));
  EXPECT_EQ(pixels[0].blue, Bits_8(50));
  EXPECT_EQ(pixels[0].alpha, Bits_8(255));
  EXPECT_EQ(pixels[4].red, Bits_8(100));
  EXPECT_EQ(pixels[4].green, Bits_8(200));
}

PERIMORTEM_UNIT_TEST(SerializationPng, decode_invalid) {
  constexpr Static::Bytes<8> garbage = {
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
  };
  auto image = Format::Png::decode(garbage);
  EXPECT_EQ(image.get_width(), Bits_32(0));
  EXPECT_EQ(image.get_height(), Bits_32(0));
}

PERIMORTEM_UNIT_TEST(SerializationPng, roundtrip_1x1) {
  Dynamic::Vector<Pixel> source_pixels;
  source_pixels.insert({0x12, 0x34, 0x56, 0x78});
  Image source_image(Data::take(source_pixels), 1, 1);

  auto encoded = Format::Png::encode(source_image);
  ASSERT(encoded.get_size() > 0);

  auto decoded = Format::Png::decode(encoded.get_view());
  EXPECT_EQ(decoded.get_width(), Bits_32(1));
  EXPECT_EQ(decoded.get_height(), Bits_32(1));

  auto decoded_pixels = decoded.get_pixels();
  ASSERT_EQ(decoded_pixels.get_size(), Count(1));
  EXPECT_EQ(decoded_pixels[0].red, Bits_8(0x12));
  EXPECT_EQ(decoded_pixels[0].green, Bits_8(0x34));
  EXPECT_EQ(decoded_pixels[0].blue, Bits_8(0x56));
  EXPECT_EQ(decoded_pixels[0].alpha, Bits_8(0x78));
}

PERIMORTEM_UNIT_TEST(SerializationPng, roundtrip_checker) {
  Dynamic::Vector<Pixel> source_pixels;
  source_pixels.insert({0xFF, 0x00, 0x00, 0xFF});
  source_pixels.insert({0x00, 0xFF, 0x00, 0xFF});
  source_pixels.insert({0x00, 0x00, 0xFF, 0xFF});
  source_pixels.insert({0xFF, 0xFF, 0xFF, 0xFF});
  Image source_image(Data::take(source_pixels), 2, 2);

  auto encoded = Format::Png::encode(source_image);
  ASSERT(encoded.get_size() > 0);

  auto decoded = Format::Png::decode(encoded.get_view());
  EXPECT_EQ(decoded.get_width(), Bits_32(2));
  EXPECT_EQ(decoded.get_height(), Bits_32(2));

  auto decoded_pixels = decoded.get_pixels();
  ASSERT_EQ(decoded_pixels.get_size(), Count(4));
  EXPECT_EQ(decoded_pixels[0].red, Bits_8(0xFF));
  EXPECT_EQ(decoded_pixels[1].green, Bits_8(0xFF));
  EXPECT_EQ(decoded_pixels[2].blue, Bits_8(0xFF));
  EXPECT_EQ(decoded_pixels[3].red, Bits_8(0xFF));
}

PERIMORTEM_UNIT_TEST(SerializationPng, roundtrip_64x64) {
  constexpr Count width = 1 << 6;
  constexpr Count height = 1 << 6;
  Dynamic::Vector<Pixel> source_pixels;
  source_pixels.resize(width * height);
  for (Count row = 0; row < height; row++) {
    for (Count col = 0; col < width; col++) {
      source_pixels[row * width + col] = {
        Bits_8(col * 8), Bits_8(row * 8), Bits_8(128), Bits_8(255)};
    }
  }
  Image source_image(Data::take(source_pixels), width, height);

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

PERIMORTEM_UNIT_TEST(SerializationPng, pixel_count_mismatch) {
  Dynamic::Vector<Pixel> one_pixel;
  one_pixel.insert({0xFF, 0x00, 0x00, 0xFF});
  Image bad_image(Data::take(one_pixel), 2, 2);
  auto encoded = Format::Png::encode(bad_image);
  EXPECT(encoded.get_size() > 0);
}

PERIMORTEM_UNIT_TEST(SerializationPng, zero_dimensions) {
  Dynamic::Vector<Pixel> empty;
  Image zero_width(Data::take(empty), 0, 1);
  Dynamic::Vector<Pixel> empty2;
  Image zero_height(Data::take(empty2), 1, 0);
  auto encoded_w = Format::Png::encode(zero_width);
  auto encoded_h = Format::Png::encode(zero_height);
  EXPECT_EQ(encoded_w.get_size(), 0);
  EXPECT_EQ(encoded_h.get_size(), 0);
}

PERIMORTEM_UNIT_TEST(SerializationPng, chunk_header_trunc) {
  // PNG with a valid IHDR that's shorter than the valid size.
  File source;
  ASSERT(source.read("validation/data/pngs/error_truncated_header.png"_view));

  auto image = Format::Png::decode(source.get_view());

  EXPECT_EQ(image.get_width(), 0);
  EXPECT(
      Test::error_contains(
          "Png: Chunk at offset 33 truncated before header end"_view));
}

PERIMORTEM_UNIT_TEST(SerializationPng, chunk_length_overrun) {
  // PNG with a chunk that claims a length of 4294967295 bytes.
  File source;
  ASSERT(source.read("validation/data/pngs/error_overrun.png"_view));

  auto image = Format::Png::decode(source.get_view());

  EXPECT_EQ(image.get_width(), 0);
  EXPECT(
      Test::error_contains(
          "Png: Chunk at offset 33 with length 100 extends past end of stream"_view));
}

PERIMORTEM_UNIT_TEST(SerializationPng, roundtrip_icon) {
  File source;
  ASSERT(source.read("validation/data/pngs/perimortem_icon.png"_view));

  auto original = Format::Png::decode(source.get_view());
  ASSERT_EQ(original.get_width(), 128);
  ASSERT_EQ(original.get_height(), 128);

  auto original_pixels = original.get_pixels();
  ASSERT_EQ(original_pixels.get_size(), Count(128 * 128));

  auto encoded = Format::Png::encode(original);
  ASSERT(encoded.get_size() > 0);

  auto decoded = Format::Png::decode(encoded.get_view());
  ASSERT_EQ(decoded.get_width(), 128);
  ASSERT_EQ(decoded.get_height(), 128);

  auto decoded_pixels = decoded.get_pixels();
  ASSERT_EQ(decoded_pixels.get_size(), Count(128 * 128));
  for (Count i = 0; i < decoded_pixels.get_size(); i++) {
    EXPECT_EQ(decoded_pixels[i].red, original_pixels[i].red);
    EXPECT_EQ(decoded_pixels[i].green, original_pixels[i].green);
    EXPECT_EQ(decoded_pixels[i].blue, original_pixels[i].blue);
    EXPECT_EQ(decoded_pixels[i].alpha, original_pixels[i].alpha);
  }
}

#if PERI_DEBUG
PERIMORTEM_UNIT_TEST(SerializationPng, crc_mismatch) {
  // PNG with corrupted chunk CRC should log the chunk type and offset so the
  // caller can identify which chunk was damaged.
  File source;
  ASSERT(source.read("validation/data/pngs/error_crc_mismatch.png"_view));

  auto image = Format::Png::decode(source.get_view());

  EXPECT_EQ(image.get_width(), 0);
  EXPECT(
      Test::error_contains(
          "Png: CRC-32 mismatch for chunk 'IHDR' at offset 8"_view));
}
#endif

PERIMORTEM_UNIT_TEST(SerializationPng, roundtrip_bloat) {
  File source;
  ASSERT(source.read("validation/data/pngs/perimortem_icon.png"_view));

  auto icon = Format::Png::decode(source.get_view());
  ASSERT_EQ(icon.get_width(), 128);
  ASSERT_EQ(icon.get_height(), 128);

  // Dynamic Huffman at depth=8 brings us close to the original, but flat-color
  // images compressed the other way by libpng can still expand slightly on
  // round-trip so we just guard against catastrophic regressions.
  auto encoded = Format::Png::encode(icon);
  ASSERT(encoded.get_size() > 0);
  EXPECT(encoded.get_size() < Count(source.get_size() * 1.15));
}
