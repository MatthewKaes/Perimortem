// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"

namespace Perimortem::Graphics::Render {

// Gives a reflected resource its stable name and location in a render
// program. Backends translate the set and slot into their native binding
// model. The name remains available to C++ tools and future editor inspection.
struct DescriptorBinding {
  Core::View::Bytes name;
  Count set = 0;
  Count slot = 0;
};

}  // namespace Perimortem::Graphics::Render
