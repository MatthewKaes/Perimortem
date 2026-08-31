// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "perimortem/graphics/sprite.hpp"

#include "perimortem/core/data.hpp"

using namespace Perimortem;

extern "C" const perimortem_object_descriptor
    TTX_DESC_Perimortem_2eGraphics__Sprite__Sprite __attribute__((weak));

const perimortem_object_descriptor Graphics::Sprite::descriptor{
  .size = sizeof(Payload),
  .alignment = alignof(Payload),
  .finalize = Graphics::Sprite::finalize,
};

Graphics::Sprite::Sprite()
    : object(perimortem_core_object_allocate(&descriptor)) {
  new (object, Core::Placement::Construct) Payload();
}

Graphics::Sprite::Sprite(const Sprite& source) : object(source.object) {
  perimortem_core_object_retain(object);
}

Graphics::Sprite::Sprite(Sprite&& source) : object(source.object) {
  source.object = nullptr;
}

Graphics::Sprite::~Sprite() {
  perimortem_core_object_release(object);
}

auto Graphics::Sprite::operator=(const Sprite& source) -> Sprite& {
  if (object == source.object) {
    return *this;
  }

  perimortem_core_object_retain(source.object);
  perimortem_core_object_release(object);
  object = source.object;
  return *this;
}

auto Graphics::Sprite::operator=(Sprite&& source) -> Sprite& {
  if (this == &source) {
    return *this;
  }

  perimortem_core_object_release(object);
  object = source.object;
  source.object = nullptr;
  return *this;
}

auto Graphics::Sprite::get_texture() const -> const Texture2D& {
  return get_payload().texture;
}

auto Graphics::Sprite::set_texture(const Texture2D& texture) -> void {
  get_payload().texture = texture;
}

auto Graphics::Sprite::get_material() const
    -> const perimortem_implementation* {
  return &get_payload().material;
}

auto Graphics::Sprite::set_material(perimortem_implementation* material)
    -> void {
  perimortem_core_implementation_move(&get_payload().material, material);
}

auto Graphics::Sprite::get_size() const -> Size2D {
  return get_payload().size;
}

auto Graphics::Sprite::set_size(Size2D size) -> void {
  get_payload().size = size;
}

auto Graphics::Sprite::get_transform() const -> Transform2D {
  return get_payload().transform;
}

auto Graphics::Sprite::set_transform(Transform2D transform) -> void {
  get_payload().transform = transform;
}

auto Graphics::Sprite::is_visible() const -> Bool {
  return get_payload().visible;
}

auto Graphics::Sprite::set_visible(Bool visible) -> void {
  get_payload().visible = visible;
}

auto Graphics::Sprite::get_z_index() const -> S64 {
  return get_payload().z_index;
}

auto Graphics::Sprite::set_z_index(S64 z_index) -> void {
  get_payload().z_index = z_index;
}

auto Graphics::Sprite::is_drawable() const -> Bool {
  const Payload& payload = get_payload();
  return payload.visible && payload.size.width != 0 &&
         payload.size.height != 0 && payload.texture.is_drawable() &&
         perimortem_core_implementation_is_valid(&payload.material);
}

auto Graphics::Sprite::retain(U8* object) -> Core::Option<Sprite> {
  BAIL_IF(object == nullptr);
  const perimortem_object_descriptor* generated =
      &TTX_DESC_Perimortem_2eGraphics__Sprite__Sprite;
  const perimortem_object_descriptor* selected =
      perimortem_core_object_descriptor(object);
  BAIL_IF(
      selected != &Sprite::descriptor &&
      (generated == nullptr || selected != generated));
  perimortem_core_object_retain(object);
  return Sprite(object);
}

auto Graphics::Sprite::finalize(U8* payload) -> void {
  Payload* selected = Core::Data::cast<Payload>(payload);
  perimortem_core_implementation_release(&selected->material);
  selected->~Payload();
}

auto Graphics::Sprite::get_payload() -> Payload& {
  return *Core::Data::cast<Payload>(object);
}

auto Graphics::Sprite::get_payload() const -> const Payload& {
  return *Core::Data::cast<const Payload>(object);
}
