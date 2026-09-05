// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "ttx/concept/none.hpp"

void Ttx::Concept::None::domain(ttx_abstract, ttx_domain_result result) const {
  result.operations->none(result);
}

auto Ttx::Concept::None::get_none() -> const None& {
  static const None none;
  return none;
}
