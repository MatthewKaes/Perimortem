// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/perimortem.hpp"

namespace Perimortem::Vulkan::Description {

// Topology is the generated Vulkan realization of one target neutral Render
// primitive topology.
enum class Topology : U8 {
  TriangleList,
};

}  // namespace Perimortem::Vulkan::Description
