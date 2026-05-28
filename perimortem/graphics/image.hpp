// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/vector.hpp"
#include "perimortem/core/data.hpp"
#include "perimortem/core/math.hpp"
#include "perimortem/core/perimortem.hpp"

#include "perimortem/memory/dynamic/vector.hpp"

#include "perimortem/graphics/pixel.hpp"

namespace Perimortem::Graphics {

// A raster image stored as RGBA pixels in row-major order.
// An empty or invalid image has width and height of zero.
// Addressing sets the behavior for out of range pixels.
class Image {
 public:
  enum class AddressingMode : Bits_8 {
    Zero,
    Clamp,
    Wrap,
  };

  Image() = default;
  Image(
      Bits_32 width,
      Bits_32 height,
      Bits_8 color_depth = 8,
      AddressingMode addressing = AddressingMode::Zero)
      : width(width),
        height(height),
        pixels(width * height),
        color_depth(color_depth) {
    pixels.forgetful_resize(width * height);
    auto bytes = pixels.get_access().get_bytes();
    Core::Data::set(bytes.get_data(), 0x00, bytes.get_size());
  }

  Image(
      Bits_32 width,
      Bits_32 height,
      Memory::Dynamic::Vector<Pixel> pixels,
      Bits_8 color_depth = 8,
      AddressingMode addressing = AddressingMode::Zero)
      : width(width),
        height(height),
        pixels(Core::Data::take(pixels)),
        color_depth(color_depth) {
    if (pixels.get_size() != width * height) {
      pixels.resize(width * height);
    }
  }

  auto get_width() const -> Bits_32 { return width; }
  auto get_height() const -> Bits_32 { return height; }
  // Bits per channel as stored in the source image header (e.g. 8 for 8bpc).
  auto get_color_depth() const -> Bits_8 { return color_depth; }

  // Used for getting raw Pixel data for optimized operations.
  auto get_pixels() const -> Core::View::Vector<Pixel> {
    return pixels.get_view();
  }

  // Returns the pixel at column x, row y with [0, 0] represents the top left
  // corner.
  //
  // Negative values are valid given the addressing mode.
  auto get_pixel(Int x, Int y) const -> Pixel {
    switch (addressing) {
    case AddressingMode::Zero:
      if (x < 0 || x > width || y < 0 || y > height) {
        return Pixel(0);
      }
      break;

    case AddressingMode::Clamp:
      x = Core::Math::clamp(x, Int(0), Int(width - 1));
      y = Core::Math::clamp(y, Int(0), Int(height - 1));
      break;

    case AddressingMode::Wrap:
      x = Core::Math::wrap(x, Int(width));
      y = Core::Math::wrap(y, Int(height));
      break;
    }
    return pixels.get_view()[Count(y) * Count(width) + Count(x)];
  }

 private:
  Bits_32 width = 0;
  Bits_32 height = 0;
  Memory::Dynamic::Vector<Pixel> pixels;
  Bits_8 color_depth = 8;
  AddressingMode addressing;
};

}  // namespace Perimortem::Graphics
