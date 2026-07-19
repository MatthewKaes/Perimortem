// Perimortem Engine
// Copyright © Matt Kaes

#include "ttx/concept/documentation.hpp"

auto Ttx::Concept::Documentation::get_empty() -> const Documentation& {
  static constexpr Documentation documentation;
  return documentation;
}
