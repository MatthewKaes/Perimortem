// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "perimortem/graphics/image.hpp"

#include "perimortem/core/data.hpp"
#include "perimortem/core/math.hpp"

using namespace Perimortem;

static auto create_pixels(
    const Memory::Dynamic::Vector<Graphics::Pixel>& source,
    Count count) -> Core::Object<Graphics::Pixel> {
  Core::Object<Graphics::Pixel> pixels(count);
  auto target = pixels.get_access();
  Count retained = Core::Math::min(source.get_size(), count);
  for (Count index = 0; index < retained; index++) {
    target.get_data()[index] = source[index];
  }
  return pixels;
}

Graphics::Image::Image(U32 width, U32 height, Addressing addressing)
    : pixels(Count(width) * Count(height)),
      pixel_count(Count(width) * Count(height)),
      size_pixels({width, height}),
      addressing(addressing) {}

Graphics::Image::Image(
    Memory::Dynamic::Vector<Pixel>&& source,
    U32 width,
    U32 height,
    Addressing addressing)
    : pixels(create_pixels(source, Count(width) * Count(height))),
      pixel_count(Count(width) * Count(height)),
      size_pixels({width, height}),
      addressing(addressing) {}

auto Graphics::Image::get_width() const -> U32 {
  return size_pixels.width;
}

auto Graphics::Image::get_height() const -> U32 {
  return size_pixels.height;
}

auto Graphics::Image::get_size_pixels() const -> Size2D {
  return {get_width(), get_height()};
}

auto Graphics::Image::get_pixels() const -> Core::View::Vector<Pixel> {
  return pixels.get_view().slice(0, pixel_count);
}

auto Graphics::Image::get_pixel(S32 x, S32 y) const -> Pixel {
  if (size_pixels.width == 0 || size_pixels.height == 0) {
    return Pixel();
  }

  switch (addressing) {
  case Addressing::Zero:
    if (x < 0 || x >= size_pixels.width || y < 0 || y >= size_pixels.height) {
      return Pixel();
    }
    break;
  case Addressing::Clamp:
    x = Core::Math::clamp(x, S32(0), S32(size_pixels.width - 1));
    y = Core::Math::clamp(y, S32(0), S32(size_pixels.height - 1));
    break;
  case Addressing::Wrap:
    x = Core::Math::wrap(x, S32(size_pixels.width));
    y = Core::Math::wrap(y, S32(size_pixels.height));
    break;
  }

  return pixels.get_view()
      .get_data()[Count(y) * Count(size_pixels.width) + Count(x)];
}

auto Graphics::Image::is_drawable() const -> Bool {
  return size_pixels.width != 0 && size_pixels.height != 0 &&
         pixel_count == Count(size_pixels.width) * Count(size_pixels.height) &&
         pixels.get_capacity() >= pixel_count;
}
