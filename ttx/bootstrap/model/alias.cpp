// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "ttx/bootstrap/model/alias.hpp"

using namespace Perimortem::Core;

auto Ttx::Model::Alias::prove(Concept::Abstract& candidate) -> Option<Alias&> {
  ttx_alias_view view;
  return ttx_alias_prove(candidate.get_abi(), &view)
             ? Option<Alias&>(static_cast<Alias&>(candidate))
             : Option<Alias&>();
}

auto Ttx::Model::Alias::prove(const Concept::Abstract& candidate)
    -> Option<const Alias&> {
  ttx_alias_view view;
  return ttx_alias_prove(candidate.get_abi(), &view)
             ? Option<const Alias&>(static_cast<const Alias&>(candidate))
             : Option<const Alias&>();
}

auto Ttx::Model::Alias::negotiate_interface(
    const ttx_abstract* requirement) const -> ttx_interface {
  return requirement == ttx_alias_requirement()
             ? ttx_interface_satisfied(
                   requirement, get_abi(), ttx_interface_marker_operations())
             : Abstract::negotiate_interface(requirement);
}
