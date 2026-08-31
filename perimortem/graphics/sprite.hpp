// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/implementation.h"
#include "perimortem/core/object.h"
#include "perimortem/core/option.hpp"

#include "perimortem/graphics/size_2d.hpp"
#include "perimortem/graphics/texture_2d.hpp"
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

  auto get_texture() const -> const Texture2D&;
  auto set_texture(const Texture2D& texture) -> void;
  auto get_material() const -> const perimortem_implementation*;
  auto set_material(perimortem_implementation* material) -> void;
  auto get_size() const -> Size2D;
  auto set_size(Size2D size) -> void;
  auto get_transform() const -> Transform2D;
  auto set_transform(Transform2D transform) -> void;
  auto is_visible() const -> Bool;
  auto set_visible(Bool visible) -> void;
  auto get_z_index() const -> S64;
  auto set_z_index(S64 z_index) -> void;
  auto is_drawable() const -> Bool;
  // Frame collection borrows this carrier and retains the real Sprite rather
  // than manufacturing a second runtime node.
  constexpr auto get_object() const -> U8* { return object; }

  // Retains this exact native Sprite after validating an erased Object carrier.
  // Runtime traversal keeps the carrier intact, so this conversion never
  // guesses ownership or allocator provenance from a payload address.
  static auto retain(U8* object) -> Core::Option<Sprite>;

 private:
  class Payload {
   public:
    perimortem_implementation material = {};
    Transform2D transform;
    Bool visible = True;
    S64 z_index = 0;
    Texture2D texture;
    Size2D size;
  };
  static_assert(__builtin_offsetof(Payload, material) == 0);
  static_assert(__builtin_offsetof(Payload, transform) == 16);
  static_assert(__builtin_offsetof(Payload, visible) == 56);
  static_assert(__builtin_offsetof(Payload, z_index) == 64);
  static_assert(__builtin_offsetof(Payload, texture) == 72);
  static_assert(__builtin_offsetof(Payload, size) == 88);
  static_assert(sizeof(Payload) == 96);

  explicit Sprite(U8* object) : object(object) {}
  static auto finalize(U8* payload) -> void;
  auto get_payload() -> Payload&;
  auto get_payload() const -> const Payload&;

  static const perimortem_object_descriptor descriptor;
  U8* object = nullptr;
};

static_assert(sizeof(Sprite) == sizeof(U8*));
static_assert(alignof(Sprite) == alignof(U8*));

}  // namespace Perimortem::Graphics
