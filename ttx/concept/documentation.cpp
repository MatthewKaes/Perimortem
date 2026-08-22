// Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "ttx/concept/documentation.hpp"

auto Ttx::Concept::Documentation::get_empty() -> const Documentation& {
  static constexpr Documentation documentation;
  return documentation;
}
