// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "perimortem/graphics/sprite.hpp"

#include "perimortem/core/data.hpp"

using namespace Perimortem;

const Core::Object<>::Descriptor Graphics::Sprite::descriptor(
    sizeof(Payload),
    alignof(Payload),
    Graphics::Sprite::finalize);

Graphics::Sprite::Sprite() : object(Core::Object<>::create(descriptor)) {
  new (object.get_payload(), Core::Placement::Construct) Payload();
}

Graphics::Sprite::Sprite(const Sprite& source) : object(source.object) {
  object.retain();
}

Graphics::Sprite::Sprite(Sprite&& source) : object(source.object) {
  source.object = Core::Object<>();
}

Graphics::Sprite::~Sprite() {
  object.release();
}

auto Graphics::Sprite::operator=(const Sprite& source) -> Sprite& {
  if (object.get_payload() == source.object.get_payload()) {
    return *this;
  }

  source.object.retain();
  object.release();
  object = source.object;
  return *this;
}

auto Graphics::Sprite::operator=(Sprite&& source) -> Sprite& {
  if (this == &source) {
    return *this;
  }

  object.release();
  object = source.object;
  source.object = Core::Object<>();
  return *this;
}

auto Graphics::Sprite::get_image() const -> const Image& {
  return get_payload().image;
}

auto Graphics::Sprite::set_image(const Image& image) -> void {
  get_payload().image = image;
}

auto Graphics::Sprite::get_size_pixels() const -> Size2D {
  return get_payload().size_pixels;
}

auto Graphics::Sprite::set_size_pixels(Size2D size) -> void {
  get_payload().size_pixels = size;
}

auto Graphics::Sprite::get_transform() const -> Transform2D {
  return get_payload().transform;
}

auto Graphics::Sprite::set_transform(Transform2D transform) -> void {
  get_payload().transform = transform;
}

auto Graphics::Sprite::get_tone() const -> Tone {
  return get_payload().tone;
}

auto Graphics::Sprite::set_tone(Tone tone) -> void {
  get_payload().tone = tone;
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
  return payload.visible && payload.size_pixels.width != 0 &&
         payload.size_pixels.height != 0 && payload.image.is_drawable();
}

auto Graphics::Sprite::retain(Core::Object<> object) -> Core::Option<Sprite> {
  BAIL_IF(object.is_empty() || &object.get_descriptor() != &Sprite::descriptor);
  object.retain();
  return Sprite(object);
}

auto Graphics::Sprite::finalize(U8* payload) -> void {
  Core::Data::cast<Payload>(payload)->~Payload();
}

auto Graphics::Sprite::get_payload() -> Payload& {
  return *Core::Data::cast<Payload>(object.get_payload());
}

auto Graphics::Sprite::get_payload() const -> const Payload& {
  return *Core::Data::cast<const Payload>(object.get_payload());
}
