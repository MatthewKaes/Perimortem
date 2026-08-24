// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/graphics/sprite_descriptor.hpp"

#include "perimortem/core/data.hpp"
#include "perimortem/core/object.hpp"

#include "perimortem/graphics/frame/sprite.hpp"
#include "perimortem/graphics/sprite.hpp"

using namespace Perimortem::Core;
using namespace Tetrodotoxin;

Graphics::SpriteDescriptor::SpriteDescriptor(
    Perimortem::Graphics::Frame::Program program)
    : descriptor(
          program.get_locator(),
          read_placement,
          nullptr,
          nullptr,
          read_draw_count,
          read_draw) {}

auto Graphics::SpriteDescriptor::read_placement(const U8*, Object<> object)
    -> Descriptor::Placement {
  auto sprite = Perimortem::Graphics::Sprite::retain(object);
  if (!sprite) {
    return {};
  }
  return Descriptor::Placement(
      sprite->get_transform().to_frame(), sprite->is_visible(),
      sprite->get_z_index());
}

auto Graphics::SpriteDescriptor::read_draw_count(const U8*, Object<> object)
    -> Count {
  auto sprite = Perimortem::Graphics::Sprite::retain(object);
  return sprite && sprite->is_drawable() ? 1 : 0;
}

auto Graphics::SpriteDescriptor::read_draw(
    const U8* product,
    Object<> object,
    Count index) -> Descriptor::Draw {
  auto sprite = Perimortem::Graphics::Sprite::retain(object);
  if (!sprite || !sprite->is_drawable() || index != 0) {
    return {};
  }

  const Perimortem::Graphics::Frame::Sprite frame = {
    sprite->get_image().get_size_pixels(),
    sprite->get_size_pixels(),
    sprite->get_tone(),
  };
  Perimortem::Memory::Dynamic::Vector<Perimortem::Graphics::Frame::Resource>
      resources;
  resources.emplace(
      Perimortem::Graphics::Frame::Resource::retain_image(sprite->get_image()));
  return Descriptor::Draw(
      Perimortem::Graphics::Frame::Program(product),
      static_cast<Perimortem::Memory::Dynamic::Vector<
          Perimortem::Graphics::Frame::Resource>&&>(resources),
      View::Bytes(Data::cast<const U8>(&frame), sizeof(frame)), 6, 0);
}
