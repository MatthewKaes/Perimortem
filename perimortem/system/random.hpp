// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/perimortem.hpp"

namespace Perimortem::System {

class Random {
 public:
  // Generates a random number from the internal Philox that is localized to the
  // current thread.
  static auto generate() -> Bits_64;

  // Read a random value directly from what ever entropy source Perimortem uses
  // for the system. This can be slow so prefer to generate a number rather than
  // reading from the system.
  static auto read_entropy() -> Bits_64;
};

}  // namespace Perimortem::System
