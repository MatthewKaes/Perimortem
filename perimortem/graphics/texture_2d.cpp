// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "perimortem/graphics/texture_2d.hpp"

#include "perimortem/core/data.hpp"

using namespace Perimortem;

extern "C" const Core::Object<>::Descriptor
    TTX_DESC_Perimortem_2eGraphics__Texture2D__Texture2D __attribute__((weak));

const Core::Object<>::Descriptor Graphics::Texture2D::descriptor(
    sizeof(Payload),
    alignof(Payload),
    Graphics::Texture2D::finalize);

Graphics::Texture2D::Texture2D() : object(Core::Object<>::create(descriptor)) {
  new (object.get_payload(), Core::Placement::Construct) Payload();
}

Graphics::Texture2D::Texture2D(const Image& image) : Texture2D() {
  get_payload().image = image;
}

Graphics::Texture2D::Texture2D(const Texture2D& source)
    : object(source.object) {
  object.retain();
}

Graphics::Texture2D::Texture2D(Texture2D&& source) : object(source.object) {
  source.object = Core::Object<>();
}

Graphics::Texture2D::~Texture2D() {
  object.release();
}

auto Graphics::Texture2D::operator=(const Texture2D& source) -> Texture2D& {
  if (object.get_payload() == source.object.get_payload()) {
    return *this;
  }

  source.object.retain();
  object.release();
  object = source.object;
  return *this;
}

auto Graphics::Texture2D::operator=(Texture2D&& source) -> Texture2D& {
  if (this == &source) {
    return *this;
  }

  object.release();
  object = source.object;
  source.object = Core::Object<>();
  return *this;
}

auto Graphics::Texture2D::get_image() const -> const Image& {
  return get_payload().image;
}

auto Graphics::Texture2D::get_size_pixels() const -> Size2D {
  return get_image().get_size_pixels();
}

auto Graphics::Texture2D::is_drawable() const -> Bool {
  return get_image().is_drawable();
}

auto Graphics::Texture2D::retain(Core::Object<> object)
    -> Core::Option<Texture2D> {
  BAIL_IF(object.is_empty());
  const Core::Object<>::Descriptor* generated =
      &TTX_DESC_Perimortem_2eGraphics__Texture2D__Texture2D;
  const Core::Object<>::Descriptor& selected = object.get_descriptor();
  BAIL_IF(
      &selected != &descriptor &&
      (generated == nullptr || &selected != generated));
  object.retain();
  return Texture2D(object);
}

auto Graphics::Texture2D::finalize(U8* payload) -> void {
  Core::Data::cast<Payload>(payload)->~Payload();
}

auto Graphics::Texture2D::get_payload() -> Payload& {
  return *Core::Data::cast<Payload>(object.get_payload());
}

auto Graphics::Texture2D::get_payload() const -> const Payload& {
  return *Core::Data::cast<const Payload>(object.get_payload());
}
