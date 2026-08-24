// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/object.hpp"
#include "perimortem/core/option.hpp"

#include "perimortem/graphics/image.hpp"
#include "perimortem/graphics/size_2d.hpp"
#include "perimortem/graphics/tone.hpp"
#include "perimortem/graphics/transform_2d.hpp"

namespace Perimortem::Graphics {

// Sprite is the native reference for the standard hosted image Object. It
// keeps semantic state in one worker local identity while frame collection
// copies the exact values needed for presentation.
class Sprite {
 public:
  Sprite();
  Sprite(const Sprite& source);
  Sprite(Sprite&& source);
  ~Sprite();

  auto operator=(const Sprite& source) -> Sprite&;
  auto operator=(Sprite&& source) -> Sprite&;

  auto get_image() const -> const Image&;
  auto set_image(const Image& image) -> void;
  auto get_size_pixels() const -> Size2D;
  auto set_size_pixels(Size2D size) -> void;
  auto get_transform() const -> Transform2D;
  auto set_transform(Transform2D transform) -> void;
  auto get_tone() const -> Tone;
  auto set_tone(Tone tone) -> void;
  auto is_visible() const -> Bool;
  auto set_visible(Bool visible) -> void;
  auto get_z_index() const -> S64;
  auto set_z_index(S64 z_index) -> void;
  auto is_drawable() const -> Bool;
  // Frame collection borrows this carrier and retains the real Sprite rather
  // than manufacturing a second runtime node.
  constexpr auto get_object() const -> Core::Object<> { return object; }

  // Retains this exact native Sprite after validating an erased Object carrier.
  // Runtime traversal keeps the carrier intact, so this conversion never
  // guesses ownership or allocator provenance from a payload address.
  static auto retain(Core::Object<> object) -> Core::Option<Sprite>;

 private:
  class Payload {
   public:
    Image image;
    Size2D size_pixels;
    Transform2D transform;
    Tone tone;
    Bool visible = True;
    S64 z_index = 0;
  };
  static_assert(__builtin_offsetof(Payload, image) == 0);
  static_assert(__builtin_offsetof(Payload, size_pixels) == sizeof(Image));
  static_assert(
      __builtin_offsetof(Payload, transform) == sizeof(Image) + sizeof(Size2D));
  static_assert(
      __builtin_offsetof(Payload, tone) ==
      sizeof(Image) + sizeof(Size2D) + sizeof(Transform2D));
  static_assert(__builtin_offsetof(Payload, visible) == 112);
  static_assert(__builtin_offsetof(Payload, z_index) == 120);
  static_assert(sizeof(Payload) == 128);

  explicit Sprite(Core::Object<> object) : object(object) {}
  static auto finalize(U8* payload) -> void;
  auto get_payload() -> Payload&;
  auto get_payload() const -> const Payload&;

  static const Core::Object<>::Descriptor descriptor;
  Core::Object<> object;
};

static_assert(sizeof(Sprite) == sizeof(U8*));
static_assert(alignof(Sprite) == alignof(U8*));

}  // namespace Perimortem::Graphics
