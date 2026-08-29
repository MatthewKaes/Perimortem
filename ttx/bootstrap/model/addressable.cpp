// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "ttx/bootstrap/model/addressable.hpp"

using namespace Perimortem::Core;

auto Ttx::Model::Addressable::prove(Concept::Abstract& candidate)
    -> Option<Addressable&> {
  ttx_addressable_view view;
  return ttx_addressable_prove(candidate.get_abi(), &view)
             ? Option<Addressable&>(static_cast<Addressable&>(candidate))
             : Option<Addressable&>();
}

auto Ttx::Model::Addressable::prove(const Concept::Abstract& candidate)
    -> Option<const Addressable&> {
  ttx_addressable_view view;
  return ttx_addressable_prove(candidate.get_abi(), &view)
             ? Option<const Addressable&>(
                   static_cast<const Addressable&>(candidate))
             : Option<const Addressable&>();
}

static auto satisfied(const ttx_abstract*, const ttx_abstract*)
    -> ttx_interface_relation {
  return TTX_INTERFACE_SATISFIED;
}

static auto type(const ttx_abstract* abstract) -> const ttx_abstract* {
  const auto& selected = static_cast<const Ttx::Model::Addressable&>(
      Ttx::Concept::Abstract::from_abi(abstract));
  return selected.get_type().get_abi();
}

static const ttx_addressable_operations operations = {
  .interface = {.negotiate = satisfied},
  .type = type,
};

auto Ttx::Model::Addressable::negotiate_interface(
    const ttx_abstract* requirement) const -> ttx_interface {
  return requirement == ttx_addressable_requirement()
             ? ttx_interface_satisfied(
                   requirement, get_abi(), &operations.interface)
             : Abstract::negotiate_interface(requirement);
}
