// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/perimortem.hpp"

namespace Perimortem::Graphics::Render {

// Identifies the graphics pipeline stage that consumes a shader module or a
// range of host-provided inputs. The values describe Graphics concepts and do
// not reuse the numeric flags of any rendering backend.
enum class Stage : U32 {
  Vertex,
  Pixel,
};

}  // namespace Perimortem::Graphics::Render
