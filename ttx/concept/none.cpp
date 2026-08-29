// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "ttx/concept/none.hpp"

auto Ttx::Concept::None::get_none() -> const None& {
  static constexpr None none;
  return none;
}
