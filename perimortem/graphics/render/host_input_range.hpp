// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/vector.hpp"

#include "perimortem/graphics/render/stage.hpp"

namespace Perimortem::Graphics::Render {

// Describes one byte range of host data made visible to the listed shader
// stages. The range is layout metadata only; the bytes supplied for a draw are
// a separate transaction and are never retained by Program.
struct HostInputRange {
  Count offset = 0;
  Count size = 0;
  Core::View::Vector<Stage> stages;
};

}  // namespace Perimortem::Graphics::Render
