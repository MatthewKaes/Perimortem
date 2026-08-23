// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "ttx/concept/invalid.hpp"

auto Ttx::Concept::Invalid::get_invalid() -> const Invalid& {
  static constexpr Invalid invalid;
  return invalid;
}
