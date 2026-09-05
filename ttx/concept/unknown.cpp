// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "ttx/concept/unknown.hpp"

void Ttx::Concept::Unknown::domain(ttx_abstract, ttx_domain_result result)
    const {
  result.operations->unknown(result);
}

void Ttx::Concept::Unknown::callable(ttx_abstract, ttx_callable_result result)
    const {
  result.operations->unknown(result);
}

void Ttx::Concept::Unknown::route(ttx_abstract, ttx_route_result result) const {
  result.operations->unknown(result);
}

void Ttx::Concept::Unknown::finite_extent(
    ttx_abstract,
    ttx_finite_extent_result result) const {
  result.operations->unknown(result);
}

void Ttx::Concept::Unknown::bytes(ttx_abstract, ttx_bytes_result result) const {
  result.operations->unknown(result);
}

auto Ttx::Concept::Unknown::negotiate(ttx_abstract) const
    -> ttx_interface_relation {
  return TTX_INTERFACE_UNKNOWN;
}

auto Ttx::Concept::Unknown::get_unknown() -> const Unknown& {
  static const Unknown unknown;
  return unknown;
}
