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
// The only supported format is currently 8 bits per channel.
class Image {
 public:
  enum class Addressing : Unsigned_8 {
    Zero,
    Clamp,
    Wrap,
  };

  Image() = default;
  Image(
      Unsigned_32 width,
      Unsigned_32 height,
      Addressing addressing = Addressing::Zero)
      : pixels(Count(width) * Count(height)),
        width(width),
        height(height),
        addressing(addressing) {
    pixels.forgetful_resize(Count(width) * Count(height));
    auto bytes = pixels.get_access().get_bytes();
    Core::Data::set(bytes.get_data(), 0x00, bytes.get_size());
  }

  Image(
      Memory::Dynamic::Vector<Pixel>&& source,
      Unsigned_32 width,
      Unsigned_32 height,
      Addressing addressing = Addressing::Zero)
      : pixels(Core::Data::take(source)),
        width(width),
        height(height),
        addressing(addressing) {
    const Count target_size = Count(width) * Count(height);
    if (pixels.get_size() != target_size) {
      const auto original_size = pixels.get_size();
      pixels.resize(target_size);

      // Clear out the new size if any.
      if (original_size < target_size) {
        auto bytes = pixels.get_access().get_bytes();
        Core::Data::set(
            bytes.get_data() + original_size * sizeof(Pixel), 0,
            (bytes.get_size() - original_size) * sizeof(Pixel));
      }
    }
  }

  auto get_width() const -> Unsigned_32 { return width; }
  auto get_height() const -> Unsigned_32 { return height; }

  // Returns the contiguous row-major pixel buffer.
  auto get_pixels() const -> Core::View::Vector<Pixel> {
    return pixels.get_view();
  }

  // Returns the pixel at column x, row y. [0, 0] is the top-left corner, x
  // increases to the right, and y increases downward.
  //
  // Negative values are valid given the addressing mode which allows for
  // different wrapping modes.
  //
  // Use get_pixels() for operations that process the buffer in bulk.
  auto get_pixel(Signed_32 x, Signed_32 y) const -> Pixel {
    if (width == 0 || height == 0) {
      return Pixel();
    }

    switch (addressing) {
      // Any out of bounds values are saturated to Unsigned_8(0)
    case Addressing::Zero:
      if (x < 0 || x >= width || y < 0 || y >= height) {
        return Pixel();
      }

      break;

      // Any out of bounds values are clamped to the edges of the image.
    case Addressing::Clamp:
      x = Core::Math::clamp(x, Signed_32(0), Signed_32(width - 1));
      y = Core::Math::clamp(y, Signed_32(0), Signed_32(height - 1));
      break;

      // Performs domain wrapping for both X and Y.
    case Addressing::Wrap:
      x = Core::Math::wrap(x, Signed_32(width));
      y = Core::Math::wrap(y, Signed_32(height));
      break;
    }

    return pixels.get_view()[Count(y) * Count(width) + Count(x)];
  }

  // Returns the number of bits that are used to represent a single value of any
  // given channel.
  static constexpr auto get_color_depth() -> Unsigned_8 { return color_depth; }

  // The number of channels used per logical pixel.
  //
  // The size of a logical pixel in bits is equal to the image's color depth
  // multiplied by the number of channels.
  static constexpr auto get_channel_count() -> Unsigned_8 {
    return channel_count;
  }

 private:
  // Currently only 8 bit is supported.
  static constexpr Unsigned_8 color_depth = 8;
  // Currently only RGBA is supported.
  static constexpr Unsigned_8 channel_count = 4;

  Memory::Dynamic::Vector<Pixel> pixels;
  Unsigned_32 width = 0;
  Unsigned_32 height = 0;
  Addressing addressing = Addressing::Zero;
};

}  // namespace Perimortem::Graphics
