// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "perimortem/graphics/formats/png.hpp"

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/data.hpp"

#include "perimortem/abi/core/object.hpp"
#include "perimortem/abi/core/option.hpp"
#include "perimortem/graphics/image.hpp"

using namespace Perimortem;

struct PngBytes {
  const U8* data;
  U64 size;
};

struct PngSize2D {
  U32 width;
  U32 height;
};

class PngImage {
 public:
  static constexpr auto create(
      const Graphics::Pixel* pixels,
      Count pixel_count,
      PngSize2D size_pixels,
      U8 addressing) -> PngImage {
    PngImage result = {};
    result.pixels = pixels;
    result.pixel_count = pixel_count;
    result.size_pixels = size_pixels;
    result.addressing = addressing;
    return result;
  }

 private:
  const Graphics::Pixel* pixels;
  Count pixel_count;
  PngSize2D size_pixels;
  U8 addressing;
};

static_assert(sizeof(PngBytes) == sizeof(U8*) + sizeof(U64));
static_assert(alignof(PngBytes) == alignof(U64));
static_assert(__is_trivial(PngBytes));
static_assert(__is_standard_layout(PngBytes));
static_assert(sizeof(PngImage) == sizeof(Graphics::Image));
static_assert(alignof(PngImage) == alignof(Graphics::Image));
static_assert(__is_trivial(PngImage));
static_assert(__is_standard_layout(PngImage));
static_assert(
    sizeof(Abi::Core::Option<PngImage>) ==
    sizeof(Core::Option<Graphics::Image>));
static_assert(
    alignof(Abi::Core::Option<PngImage>) ==
    alignof(Core::Option<Graphics::Image>));

extern "C" auto perimortem_graphics_png_decode(PngBytes source)
    -> Abi::Core::Option<PngImage> {
  Graphics::Image image = Graphics::Formats::Png::decode(
      Core::View::Bytes(source.data, source.size));
  if (!image.is_drawable()) {
    return Abi::Core::Option<PngImage>::create();
  }

  Core::View::Vector<Graphics::Pixel> pixels = image.get_pixels();
  perimortem_core_object_retain(
      Core::Data::cast<U8>(const_cast<Graphics::Pixel*>(pixels.get_data())));
  return Abi::Core::Option<PngImage>::create(
      PngImage::create(
          pixels.get_data(), pixels.get_size(),
          PngSize2D{image.get_width(), image.get_height()},
          U8(image.get_addressing())));
}
