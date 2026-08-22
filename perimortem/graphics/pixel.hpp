// Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

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
  explicit Pixel(U8 grey) : red(grey), green(grey), blue(grey), alpha(opaque) {}

  // Replicates grey to all color channels with an explicit alpha value.
  Pixel(U8 grey, U8 alpha) : red(grey), green(grey), blue(grey), alpha(alpha) {}

  // Stores three independent color channels and uses full opacity.
  Pixel(U8 red, U8 green, U8 blue)
      : red(red), green(green), blue(blue), alpha(opaque) {}

  // Stores all four channels directly.
  Pixel(U8 red, U8 green, U8 blue, U8 alpha)
      : red(red), green(green), blue(blue), alpha(alpha) {}

  static constexpr auto get_bit_depth() -> Count { return 8; }
  static constexpr auto get_byte_count() -> Count { return 4; }

  U8 red = 0;
  U8 green = 0;
  U8 blue = 0;
  U8 alpha = 0;

 private:
  static constexpr U8 opaque = 0xFF;
};

static_assert(sizeof(Pixel) == 4);

}  // namespace Perimortem::Graphics
