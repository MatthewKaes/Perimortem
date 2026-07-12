// Perimortem Engine
// Copyright © Matt Kaes

#include "perimortem/graphics/image.hpp"

#include "validation/unit_test.hpp"

#include "perimortem/core/data.hpp"
#include "perimortem/core/null_terminated.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Graphics;
using namespace Perimortem::Memory;
using namespace Validation;

static Harness GraphicsImage = {
  .name = "Graphics::Image"_view,
};

PERIMORTEM_UNIT_TEST(GraphicsImage, zero_addressing_includes_edge_coordinates) {
  Dynamic::Vector<Pixel> pixels;
  pixels.emplace(Pixel(0x11, 0x22, 0x33, 0x44));
  Image image(Data::take(pixels), 1, 1);

  const Pixel horizontal_edge = image.get_pixel(1, 0);
  const Pixel vertical_edge = image.get_pixel(0, 1);
  EXPECT_EQ(horizontal_edge.red, Bits_8(0));
  EXPECT_EQ(horizontal_edge.alpha, Bits_8(0));
  EXPECT_EQ(vertical_edge.red, Bits_8(0));
  EXPECT_EQ(vertical_edge.alpha, Bits_8(0));
}

PERIMORTEM_UNIT_TEST(GraphicsImage, clamp_addressing_uses_nearest_edge) {
  Dynamic::Vector<Pixel> pixels;
  pixels.emplace(Pixel(0x11, 0x22, 0x33, 0x44));
  pixels.emplace(Pixel(0x55, 0x66, 0x77, 0x88));
  Image image(Data::take(pixels), 2, 1, Image::Addressing::Clamp);

  const Pixel left = image.get_pixel(-1, 0);
  const Pixel right = image.get_pixel(2, 0);
  EXPECT_EQ(left.red, Bits_8(0x11));
  EXPECT_EQ(left.alpha, Bits_8(0x44));
  EXPECT_EQ(right.red, Bits_8(0x55));
  EXPECT_EQ(right.alpha, Bits_8(0x88));
}

PERIMORTEM_UNIT_TEST(GraphicsImage, wrap_addressing_uses_image_domain) {
  Dynamic::Vector<Pixel> pixels;
  pixels.emplace(Pixel(0x11, 0x22, 0x33, 0x44));
  pixels.emplace(Pixel(0x55, 0x66, 0x77, 0x88));
  Image image(Data::take(pixels), 2, 1, Image::Addressing::Wrap);

  const Pixel from_left = image.get_pixel(-1, 0);
  const Pixel from_right = image.get_pixel(2, 0);
  EXPECT_EQ(from_left.red, Bits_8(0x55));
  EXPECT_EQ(from_left.alpha, Bits_8(0x88));
  EXPECT_EQ(from_right.red, Bits_8(0x11));
  EXPECT_EQ(from_right.alpha, Bits_8(0x44));
}
