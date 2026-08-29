// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "ttx/bootstrap/concept/unknown.hpp"

using namespace Perimortem::Core;

auto Ttx::Concept::Unknown::prove(Abstract& candidate) -> Option<Unknown&> {
  ttx_unknown_view view;
  return ttx_unknown_prove(candidate.get_abi(), &view)
             ? Option<Unknown&>(static_cast<Unknown&>(candidate))
             : Option<Unknown&>();
}

auto Ttx::Concept::Unknown::prove(const Abstract& candidate)
    -> Option<const Unknown&> {
  ttx_unknown_view view;
  return ttx_unknown_prove(candidate.get_abi(), &view)
             ? Option<const Unknown&>(static_cast<const Unknown&>(candidate))
             : Option<const Unknown&>();
}

auto Ttx::Concept::Unknown::negotiate_interface(
    const ttx_abstract* requirement) const -> ttx_interface {
  return requirement == ttx_unknown_requirement()
             ? ttx_interface_satisfied(
                   requirement, get_abi(), ttx_interface_marker_operations())
             : Abstract::negotiate_interface(requirement);
}

auto Ttx::Concept::Unknown::get_unknown() -> const Unknown& {
  static constexpr Unknown unknown;
  return unknown;
}
