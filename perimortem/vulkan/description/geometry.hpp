// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/perimortem.hpp"

namespace Perimortem::Vulkan::Description {

// Geometry selects one portable geometry source published by Render and
// realized by Vulkan.
enum class Geometry : U8 {
  UnitQuad2D,
};

}  // namespace Perimortem::Vulkan::Description
