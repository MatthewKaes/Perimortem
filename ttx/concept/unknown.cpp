// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "ttx/concept/unknown.hpp"

auto Ttx::Concept::Unknown::get_unknown() -> const Unknown& {
  static constexpr Unknown unknown;
  return unknown;
}
