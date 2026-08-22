// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/perimortem.hpp"

namespace Puffer {

// Command retains one native invocation and selects its single requested mode.
class Command {
 public:
  constexpr Command(S32 argument_count, char** argument_values)
      : argument_count(argument_count), argument_values(argument_values) {}

  auto run() const -> S32;

 private:
  S32 argument_count;
  char** argument_values;
};

}  // namespace Puffer
