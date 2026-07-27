// Perimortem Engine
// Copyright © Matt Kaes

#include "ttx/concept/invalid.hpp"

auto Ttx::Concept::Invalid::get_invalid() -> const Invalid& {
  static constexpr Invalid invalid;
  return invalid;
}
