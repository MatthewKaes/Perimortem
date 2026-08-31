// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "perimortem/graphics/frame/resource.hpp"

using namespace Perimortem;

Graphics::Frame::Resource::Resource(U8* object) : object(object) {
  perimortem_core_object_retain(object);
}

auto Graphics::Frame::Resource::retain_texture(
    const Graphics::Texture2D& texture) -> Resource {
  BAIL_IF(!texture.is_drawable());
  Resource resource(texture.get_image().get_object());
  resource.sampler = texture.get_sampler();
  return resource;
}

Graphics::Frame::Resource::Resource(const Resource& source)
    : object(source.object), sampler(source.sampler) {
  perimortem_core_object_retain(object);
}

Graphics::Frame::Resource::Resource(Resource&& source)
    : object(source.object), sampler(source.sampler) {
  source.object = nullptr;
}

Graphics::Frame::Resource::~Resource() {
  perimortem_core_object_release(object);
}

auto Graphics::Frame::Resource::operator=(const Resource& source) -> Resource& {
  if (object == source.object) {
    sampler = source.sampler;
    return *this;
  }

  perimortem_core_object_retain(source.object);
  perimortem_core_object_release(object);
  object = source.object;
  sampler = source.sampler;
  return *this;
}

auto Graphics::Frame::Resource::operator=(Resource&& source) -> Resource& {
  if (this == &source) {
    return *this;
  }

  perimortem_core_object_release(object);
  object = source.object;
  sampler = source.sampler;
  source.object = nullptr;
  return *this;
}

auto Graphics::Frame::Resource::get_reservations() const -> Count {
  return perimortem_core_object_reservations(object);
}
