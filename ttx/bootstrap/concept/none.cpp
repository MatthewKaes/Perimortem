// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "ttx/bootstrap/concept/none.hpp"

using namespace Perimortem::Core;

auto Ttx::Concept::None::prove(Abstract& candidate) -> Option<None&> {
  ttx_none_view view;
  return ttx_none_prove(candidate.get_abi(), &view)
             ? Option<None&>(static_cast<None&>(candidate))
             : Option<None&>();
}

auto Ttx::Concept::None::prove(const Abstract& candidate)
    -> Option<const None&> {
  ttx_none_view view;
  return ttx_none_prove(candidate.get_abi(), &view)
             ? Option<const None&>(static_cast<const None&>(candidate))
             : Option<const None&>();
}

auto Ttx::Concept::None::negotiate_interface(
    const ttx_abstract* requirement) const -> ttx_interface {
  return requirement == ttx_none_requirement()
             ? ttx_interface_satisfied(
                   requirement, get_abi(), ttx_interface_marker_operations())
             : Constant::negotiate_interface(requirement);
}

auto Ttx::Concept::None::get_none() -> const None& {
  static constexpr None none;
  return none;
}
