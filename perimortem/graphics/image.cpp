// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "perimortem/graphics/image.hpp"

#include "perimortem/core/data.hpp"
#include "perimortem/core/math.hpp"

using namespace Perimortem;

extern "C" const perimortem_object_descriptor
    TTX_DESC_Perimortem_2eGraphics__Image__Image __attribute__((weak));

const perimortem_object_descriptor Graphics::Image::descriptor{
  .size = sizeof(Payload),
  .alignment = alignof(Payload),
  .finalize = Graphics::Image::finalize,
};

const perimortem_object_descriptor Graphics::Image::pixel_descriptor{
  .size = sizeof(Pixel),
  .alignment = alignof(Pixel),
  .finalize = perimortem_core_object_finalize_trivial,
};

auto Graphics::Image::create_pixels(
    const Memory::Dynamic::Vector<Graphics::Pixel>& source,
    Count count) -> U8* {
  if (count == 0) {
    return nullptr;
  }
  U8* pixels = perimortem_core_object_allocate_buffer(
      &Graphics::Image::pixel_descriptor, count, sizeof(Graphics::Pixel));
  Graphics::Pixel* target = Core::Data::cast<Graphics::Pixel>(pixels);
  Count retained = Core::Math::min(source.get_size(), count);
  for (Count index = 0; index < count; index++) {
    target[index] = index < retained ? source[index] : Graphics::Pixel();
  }

  return pixels;
}

Graphics::Image::Image()
    : object(perimortem_core_object_allocate(&descriptor)) {
  new (object, Core::Placement::Construct) Payload();
}

Graphics::Image::Image(U32 width, U32 height) : Image() {
  Payload& payload = get_payload();
  payload.pixel_count = Count(width) * Count(height);
  if (payload.pixel_count != 0) {
    payload.pixels = perimortem_core_object_allocate_buffer(
        &pixel_descriptor, payload.pixel_count, sizeof(Pixel));
  }
  payload.size_pixels = {width, height};
}

Graphics::Image::Image(
    Memory::Dynamic::Vector<Pixel>&& source,
    U32 width,
    U32 height)
    : Image() {
  Payload& payload = get_payload();
  payload.pixels = create_pixels(source, Count(width) * Count(height));
  payload.pixel_count = Count(width) * Count(height);
  payload.size_pixels = {width, height};
}

Graphics::Image::Image(const Image& source) : object(source.object) {
  perimortem_core_object_retain(object);
}

Graphics::Image::Image(Image&& source) : object(source.object) {
  source.object = nullptr;
}

Graphics::Image::~Image() {
  perimortem_core_object_release(object);
}

auto Graphics::Image::operator=(const Image& source) -> Image& {
  if (object == source.object) {
    return *this;
  }

  perimortem_core_object_retain(source.object);
  perimortem_core_object_release(object);
  object = source.object;
  return *this;
}

auto Graphics::Image::operator=(Image&& source) -> Image& {
  if (this == &source) {
    return *this;
  }

  perimortem_core_object_release(object);
  object = source.object;
  source.object = nullptr;
  return *this;
}

auto Graphics::Image::get_width() const -> U32 {
  return get_payload().size_pixels.width;
}

auto Graphics::Image::get_height() const -> U32 {
  return get_payload().size_pixels.height;
}

auto Graphics::Image::get_size_pixels() const -> Size2D {
  return get_payload().size_pixels;
}

auto Graphics::Image::get_pixels() const -> Core::View::Vector<Pixel> {
  const Payload& payload = get_payload();
  return Core::View::Vector<Pixel>(
      Core::Data::cast<Pixel>(payload.pixels), payload.pixel_count);
}

auto Graphics::Image::get_pixel(S32 x, S32 y) const -> Pixel {
  const Payload& payload = get_payload();
  if (x < 0 || x >= payload.size_pixels.width || y < 0 ||
      y >= payload.size_pixels.height) {
    return Pixel();
  }

  return Core::Data::cast<Pixel>(
      payload.pixels)[Count(y) * Count(payload.size_pixels.width) + Count(x)];
}

auto Graphics::Image::is_drawable() const -> Bool {
  const Payload& payload = get_payload();
  return payload.size_pixels.width != 0 && payload.size_pixels.height != 0 &&
         payload.pixel_count ==
             Count(payload.size_pixels.width) * payload.size_pixels.height &&
         perimortem_core_object_capacity(payload.pixels) >=
             payload.pixel_count * sizeof(Pixel);
}

auto Graphics::Image::retain(U8* object) -> Core::Option<Image> {
  BAIL_IF(object == nullptr);
  const perimortem_object_descriptor* generated =
      &TTX_DESC_Perimortem_2eGraphics__Image__Image;
  const perimortem_object_descriptor* selected =
      perimortem_core_object_descriptor(object);
  BAIL_IF(
      selected != &descriptor &&
      (generated == nullptr || selected != generated));
  perimortem_core_object_retain(object);
  return Image(object);
}

auto Graphics::Image::finalize(U8* payload) -> void {
  Payload* selected = Core::Data::cast<Payload>(payload);
  perimortem_core_object_release(selected->pixels);
  selected->~Payload();
}

auto Graphics::Image::get_payload() -> Payload& {
  return *Core::Data::cast<Payload>(object);
}

auto Graphics::Image::get_payload() const -> const Payload& {
  return *Core::Data::cast<const Payload>(object);
}
