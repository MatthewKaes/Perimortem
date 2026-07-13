// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/perimortem.hpp"

namespace Perimortem::Graphics {

// The canonical decoded RGBA pixel with eight bits per channel. Public channel
// fields are intentional data-oriented storage. Image algorithms can process a
// continuous Pixel buffer without accessors obscuring the four-byte layout.
// Encoded formats with another channel order or depth are converted at the
// codec boundary rather than changing the meaning of this runtime value.
class Pixel {
 public:
  // Fully transparent black is the zero state.
  Pixel() = default;

  // Replicates grey to all color channels and uses full opacity.
  explicit Pixel(Bits_8 grey)
      : red(grey), green(grey), blue(grey), alpha(opaque) {}

  // Replicates grey to all color channels with an explicit alpha value.
  Pixel(Bits_8 grey, Bits_8 alpha)
      : red(grey), green(grey), blue(grey), alpha(alpha) {}

  // Stores three independent color channels and uses full opacity.
  Pixel(Bits_8 red, Bits_8 green, Bits_8 blue)
      : red(red), green(green), blue(blue), alpha(opaque) {}

  // Stores all four channels directly.
  Pixel(Bits_8 red, Bits_8 green, Bits_8 blue, Bits_8 alpha)
      : red(red), green(green), blue(blue), alpha(alpha) {}

  static constexpr auto get_bit_depth() -> Count { return 8; }
  static constexpr auto get_byte_count() -> Count { return 4; }

  Bits_8 red = 0;
  Bits_8 green = 0;
  Bits_8 blue = 0;
  Bits_8 alpha = 0;

 private:
  static constexpr Bits_8 opaque = 0xFF;
};

static_assert(sizeof(Pixel) == 4);

}  // namespace Perimortem::Graphics
