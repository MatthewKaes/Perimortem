// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/perimortem.hpp"

namespace Perimortem::Graphics {

// A single RGBA pixel with 8 bits per channel. Used as the canonical decoded
// pixel format across all image serializers in this namespace.
//
// Pixel is one of the few cases where we bend the coding conventions and let
// callers access it's members directly. This should most likely be revisted
// but it's a hold over of every pixel implementation that more or less treats
// it as a fancy Bits_32.
//
// In practice however pixel could support different bit depths or formats so
// we'll need to refactor if that comes up in the future.
class Pixel {
 public:
  // Fully transparent black — used as the zero state.
  Pixel() = default;

  // Greyscale: replicates grey to all three color channels, fully opaque.
  explicit Pixel(Bits_8 grey)
      : red(grey), green(grey), blue(grey), alpha(opaque) {}

  // Greyscale with explicit alpha control.
  Pixel(Bits_8 grey, Bits_8 alpha)
      : red(grey), green(grey), blue(grey), alpha(alpha) {}

  // RGB: three independent color channels, fully opaque.
  Pixel(Bits_8 red, Bits_8 green, Bits_8 blue)
      : red(red), green(green), blue(blue), alpha(opaque) {}

  // RGBA: full control over all four channels.
  Pixel(Bits_8 red, Bits_8 green, Bits_8 blue, Bits_8 alpha)
      : red(red), green(green), blue(blue), alpha(alpha) {}

  static constexpr auto get_bit_depth() -> Count { return 8; };
  static constexpr auto get_byte_count() -> Count { return 4; };

  Bits_8 red = 0;
  Bits_8 green = 0;
  Bits_8 blue = 0;
  Bits_8 alpha = 0;

 private:
  static constexpr Bits_8 opaque = 0xFF;
};

static_assert(sizeof(Pixel) == 4);

}  // namespace Perimortem::Graphics
