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

// A raster image stored as RGBA pixels in row-major order that provides safe
// pixel level access as well as raw buffer access for speed.
//
// Image provides several addressing modes that sets the behavior for out of
// range pixels.
//
// Image only currently supported format is 8 bit depth RGBA.
class Image {
 public:
  enum class Addressing : Bits_8 {
    Zero,
    Clamp,
    Wrap,
  };

  Image() = default;
  Image(Bits_32 width, Bits_32 height, Addressing addressing = Addressing::Zero)
      : pixels(width * height),
        width(width),
        height(height),
        addressing(addressing) {
    pixels.forgetful_resize(width * height);
    auto bytes = pixels.get_access().get_bytes();
    Core::Data::set(bytes.get_data(), 0x00, bytes.get_size());
  }

  Image(
      Memory::Dynamic::Vector<Pixel>&& pixels,
      Bits_32 width,
      Bits_32 height,
      Addressing addressing = Addressing::Zero)
      : pixels(Core::Data::take(pixels)),
        width(width),
        height(height),
        addressing(addressing) {
    if (pixels.get_size() != width * height) {
      pixels.resize(width * height);
    }
  }

  auto get_width() const -> Bits_32 { return width; }
  auto get_height() const -> Bits_32 { return height; }

  // Returns the number of bits that are used to represent a single value of any
  // given channel.
  auto get_color_depth() const -> Bits_8 { return color_depth; }

  // The number of channels used per logical pixel.
  //
  // The size of a logical pixel in bits is equal to the image's color depth
  // multiplied by the number of channels.
  auto get_channel_count() const -> Bits_8 { return channel_count; }

  // Used for getting raw Pixel data for optimized operations.
  auto get_pixels() const -> Core::View::Vector<Pixel> {
    return pixels.get_view();
  }

  // Returns the pixel at column x, row y with [0, 0] represents the top left
  // corner following most standard conventions with increasing x going right
  // and increasing y going down.
  //
  // Negative values are valid given the addressing mode which allows for
  // different wrapping modes.
  //
  // Used when safety is prefered, but for speed most graphics operations should
  // be vectorized on the CPU or GPU and should go through `get_pixels()` to
  // directly manipulate the data.
  auto get_pixel(Int x, Int y) const -> Pixel {
    switch (addressing) {
      // Any out of bounds values are saturated to Bits_8(0)
    case Addressing::Zero:
      if (x < 0 || x > width || y < 0 || y > height) {
        return Pixel(0);
      }
      break;

      // Any out of bounds values are clamped to the edges of the image.
    case Addressing::Clamp:
      x = Core::Math::clamp(x, Int(0), Int(width - 1));
      y = Core::Math::clamp(y, Int(0), Int(height - 1));
      break;

      // Performs domain wrapping for both X and Y.
    case Addressing::Wrap:
      x = Core::Math::wrap(x, Int(width));
      y = Core::Math::wrap(y, Int(height));
      break;
    }
    return pixels.get_view()[Count(y) * Count(width) + Count(x)];
  }

 private:
  // Currently only 8 bit is supported.
  static constexpr Bits_8 color_depth = 8;
  // Currently only RGBA is supported.
  static constexpr Bits_8 channel_count = 4;

  Memory::Dynamic::Vector<Pixel> pixels;
  Bits_32 width = 0;
  Bits_32 height = 0;
  Addressing addressing;
};

}  // namespace Perimortem::Graphics
