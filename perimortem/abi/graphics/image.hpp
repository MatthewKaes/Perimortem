// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/null_terminated.hpp"

#include "perimortem/abi/core/option.hpp"
#include "perimortem/abi/graphics/size_2d.hpp"
#include "perimortem/graphics/image.hpp"

namespace Perimortem::Abi::Graphics {

// ImageBytes is the target carrier for one borrowed View[U8]. The C boundary
// keeps the two machine words explicit while the native decoder observes them
// only for the duration of its call.
struct ImageBytes {
  const U8* data;
  U64 size;
};

// Image is the trivial carrier for one decoded graphics value. The pixel
// pointer carries one Object reservation transferred to generated code, while
// pixel_count keeps allocator capacity distinct from the logical image extent.
class Image {
 public:
  static constexpr auto create(
      const Perimortem::Graphics::Pixel* pixels,
      Count pixel_count,
      Size2D size_pixels,
      U8 addressing) -> Image {
    Image result = {};
    result.pixels = pixels;
    result.pixel_count = pixel_count;
    result.size_pixels = size_pixels;
    result.addressing = addressing;
    return result;
  }

 private:
  const Perimortem::Graphics::Pixel* pixels;
  Count pixel_count;
  Size2D size_pixels;
  U8 addressing;
};

inline constexpr Perimortem::Core::View::Bytes image_decode_symbol =
    "perimortem_graphics_image_decode"_view;

static_assert(sizeof(ImageBytes) == sizeof(U8*) + sizeof(U64));
static_assert(alignof(ImageBytes) == alignof(U64));
static_assert(__is_trivial(ImageBytes));
static_assert(__is_standard_layout(ImageBytes));
static_assert(
    sizeof(Image) == sizeof(U8*) + sizeof(Count) * 2 + sizeof(Size2D));
static_assert(alignof(Image) == alignof(Count));
static_assert(__is_trivial(Image));
static_assert(__is_standard_layout(Image));
static_assert(sizeof(Image) == sizeof(Perimortem::Graphics::Image));
static_assert(alignof(Image) == alignof(Perimortem::Graphics::Image));
static_assert(
    sizeof(Perimortem::Abi::Core::Option<Image>) ==
    sizeof(Perimortem::Core::Option<Perimortem::Graphics::Image>));
static_assert(
    alignof(Perimortem::Abi::Core::Option<Image>) ==
    alignof(Perimortem::Core::Option<Perimortem::Graphics::Image>));
static_assert(Perimortem::Abi::Core::Option<Image>::get_value_offset() == 0);
static_assert(
    Perimortem::Abi::Core::Option<Image>::get_state_offset() == sizeof(Image));

}  // namespace Perimortem::Abi::Graphics

// Success transfers one pixel Object reservation inside the returned Image
// value. Absence reports malformed or unsupported encoded input.
extern "C" auto perimortem_graphics_image_decode(
    Perimortem::Abi::Graphics::ImageBytes source)
    -> Perimortem::Abi::Core::Option<Perimortem::Abi::Graphics::Image>;
