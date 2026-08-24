// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/graphics/frame/program.hpp"
#include "perimortem/vulkan/description/program.hpp"

namespace Perimortem::Vulkan::Programs {

// Sprite publishes the built in Vulkan reference pipeline as one process
// lifetime product. The matching GLSL remains beside these reviewed words so
// the target description can later adopt TTX generated modules unchanged.
class Sprite {
 public:
  static auto get_locator() -> Perimortem::Graphics::Frame::Program;
  static auto get_description() -> Description::Program;
};

}  // namespace Perimortem::Vulkan::Programs
