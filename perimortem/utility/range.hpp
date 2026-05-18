// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/perimortem.hpp"

namespace Perimortem::Utility {

// Represents a start index and size of a continuous range of some object.
struct Range {
  Count start;
  Count size;
};

}  // namespace Perimortem::Utility
