// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/graphics/frame/program.hpp"
#include "tetrodotoxin/graphics/descriptor.hpp"

namespace Tetrodotoxin::Graphics {

// SpriteDescriptor joins one native Sprite Type with its selected compiled
// Program product. The locator stays process scoped and no Shader words or
// backend handles enter the hosted Object.
class SpriteDescriptor {
 public:
  explicit SpriteDescriptor(Perimortem::Graphics::Frame::Program program);

  constexpr auto get_descriptor() const -> const Descriptor& {
    return descriptor;
  }

  static auto get_runtime_descriptor() -> const Descriptor&;

 private:
  static auto read_placement(
      const U8* product,
      Perimortem::Core::Object<> object) -> Descriptor::Placement;
  static auto read_draw_count(
      const U8* product,
      Perimortem::Core::Object<> object) -> Count;
  static auto read_draw(
      const U8* product,
      Perimortem::Core::Object<> object,
      Count index) -> Descriptor::Draw;

  Descriptor descriptor;
};

}  // namespace Tetrodotoxin::Graphics

extern "C" auto tetrodotoxin_graphics_sprite_descriptor()
    -> const Tetrodotoxin::Graphics::Descriptor*;
