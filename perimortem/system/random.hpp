// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/perimortem.hpp"

namespace Perimortem::System {

class Random {
 public:
  static auto generate() -> Bits_64;
};

}  // namespace Perimortem::System
