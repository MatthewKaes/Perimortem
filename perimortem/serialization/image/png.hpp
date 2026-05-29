// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/memory/dynamic/bytes.hpp"

#include "perimortem/graphics/image.hpp"

namespace Perimortem::Serialization::Image::Png {

// The color encoding used in a PNG file's IHDR chunk.
enum class ColorType : Bits_8 {
  Greyscale      = 0,
  Rgb            = 2,
  Indexed        = 3,
  GreyscaleAlpha = 4,
  Rgba           = 6,
};

// Metadata from a PNG file's IHDR chunk, readable without decoding pixels.
struct ImageInfo {
  Bits_32 width = 0;
  Bits_32 height = 0;
  Bits_8 bit_depth = 0;
  ColorType color_type = ColorType::Rgba;
  Bool valid = False;
};

// Reads image dimensions and color metadata without decompressing pixel data.
auto read_info(Core::View::Bytes source) -> ImageInfo;

// Decodes a Png byte stream into an Image (width × height RGBA pixels,
// row-major). All source color formats are expanded to full RGBA.
// Returns an empty Image on malformed or unsupported input.
auto decode(Core::View::Bytes source) -> Graphics::Image;

// Encodes an Image to a Png byte stream. Returns empty bytes on failure.
auto encode(const Graphics::Image& image) -> Memory::Dynamic::Bytes;

}  // namespace Perimortem::Serialization::Image::Png
