// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/view/vector.hpp"
#include "perimortem/core/object.h"
#include "perimortem/core/option.hpp"

#include "perimortem/memory/buffer.h"
#include "perimortem/memory/dynamic/vector.hpp"

#include "perimortem/graphics/pixel.hpp"
#include "perimortem/graphics/size_2d.hpp"

namespace Perimortem::Graphics {

// Image is one shared decoded RGBA identity. Copies retain the same worker
// local content, while target images and sampling policy remain independent
// runtime facts.
class Image {
 public:
  Image();
  Image(U32 width, U32 height);
  Image(Memory::Dynamic::Vector<Pixel>&& source, U32 width, U32 height);
  Image(const Image& source);
  Image(Image&& source);
  ~Image();

  auto operator=(const Image& source) -> Image&;
  auto operator=(Image&& source) -> Image&;

  auto get_width() const -> U32;
  auto get_height() const -> U32;
  auto get_size_pixels() const -> Size2D;
  // The returned pixels use row major order and borrow this Image lifetime.
  auto get_pixels() const -> Core::View::Vector<Pixel>;
  auto get_pixel(S32 x, S32 y) const -> Pixel;
  auto is_drawable() const -> Bool;
  constexpr auto get_object() const -> U8* { return object; }

  static auto retain(U8* object) -> Core::Option<Image>;

  static constexpr auto get_color_depth() -> U8 { return color_depth; }
  static constexpr auto get_channel_count() -> U8 { return channel_count; }

 private:
  class Payload {
   public:
    U8* pixels = nullptr;
    Count pixel_count = 0;
    Size2D size_pixels;
  };
  static_assert(__builtin_offsetof(Payload, pixels) == 0);
  static_assert(__builtin_offsetof(Payload, pixel_count) == sizeof(U8*));
  static_assert(
      __builtin_offsetof(Payload, size_pixels) == sizeof(U8*) + sizeof(Count));
  static_assert(
      sizeof(Payload) == sizeof(U8*) + sizeof(Count) + sizeof(Size2D));

  explicit Image(U8* object) : object(object) {}
  static auto create_pixels(
      const Memory::Dynamic::Vector<Pixel>& source,
      Count count) -> U8*;
  static auto finalize(U8* payload) -> void;
  auto get_payload() -> Payload&;
  auto get_payload() const -> const Payload&;

  static constexpr U8 color_depth = 8;
  static constexpr U8 channel_count = 4;
  static const perimortem_object_descriptor descriptor;
  static const perimortem_object_descriptor pixel_descriptor;
  U8* object = nullptr;
};

static_assert(sizeof(Image) == sizeof(U8*));
static_assert(alignof(Image) == alignof(U8*));

}  // namespace Perimortem::Graphics
