// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/view/vector.hpp"
#include "perimortem/core/object.hpp"

#include "perimortem/memory/dynamic/vector.hpp"

#include "perimortem/graphics/pixel.hpp"
#include "perimortem/graphics/size_2d.hpp"

namespace Perimortem::Graphics {

// Image is one decoded RGBA value backed by shared worker local pixel storage.
// The dimensions and addressing policy give that storage graphics meaning,
// while an empty backing Object is the ordinary default value. Image exposes no
// writable pixel access, so copies can safely retain the same immutable buffer.
class Image {
 public:
  enum class Addressing : U8 {
    Zero,
    Clamp,
    Wrap,
  };

  Image() = default;
  Image(U32 width, U32 height, Addressing addressing = Addressing::Zero);
  Image(
      Memory::Dynamic::Vector<Pixel>&& source,
      U32 width,
      U32 height,
      Addressing addressing = Addressing::Zero);
  auto get_width() const -> U32;
  auto get_height() const -> U32;
  auto get_size_pixels() const -> Size2D;
  constexpr auto get_addressing() const -> Addressing { return addressing; }
  // The returned row major pixels borrow this Image lifetime.
  auto get_pixels() const -> Core::View::Vector<Pixel>;
  // Coordinates begin at the top left and grow rightward and downward. The
  // selected Addressing policy defines observations outside that extent.
  auto get_pixel(S32 x, S32 y) const -> Pixel;
  auto is_drawable() const -> Bool;

  static constexpr auto get_color_depth() -> U8 { return color_depth; }
  static constexpr auto get_channel_count() -> U8 { return channel_count; }

 private:
  static constexpr U8 color_depth = 8;
  static constexpr U8 channel_count = 4;

  Core::Object<Pixel> pixels;
  Count pixel_count = 0;
  Size2D size_pixels;
  Addressing addressing = Addressing::Zero;
};

static_assert(
    sizeof(Image) == sizeof(U8*) + sizeof(Count) * 2 + sizeof(Size2D));
static_assert(alignof(Image) == alignof(Count));
static_assert(__is_standard_layout(Image));

}  // namespace Perimortem::Graphics
