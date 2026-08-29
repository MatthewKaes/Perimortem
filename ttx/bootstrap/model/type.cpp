// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "ttx/bootstrap/model/type.hpp"

using namespace Perimortem::Core;

auto Ttx::Model::Type::prove(Concept::Abstract& candidate) -> Option<Type&> {
  ttx_type_view view;
  return ttx_type_prove(candidate.get_abi(), &view)
             ? Option<Type&>(static_cast<Type&>(candidate))
             : Option<Type&>();
}

auto Ttx::Model::Type::prove(const Concept::Abstract& candidate)
    -> Option<const Type&> {
  ttx_type_view view;
  return ttx_type_prove(candidate.get_abi(), &view)
             ? Option<const Type&>(static_cast<const Type&>(candidate))
             : Option<const Type&>();
}

static auto satisfied(const ttx_abstract*, const ttx_abstract*)
    -> ttx_interface_relation {
  return TTX_INTERFACE_SATISFIED;
}

static auto layout(const ttx_abstract* abstract) -> const ttx_layout* {
  const auto& selected = static_cast<const Ttx::Model::Type&>(
      Ttx::Concept::Abstract::from_abi(abstract));
  return selected.get_layout().get_abi();
}

static const ttx_type_operations operations = {
  .interface = {.negotiate = satisfied},
  .layout = layout,
};

auto Ttx::Model::Type::negotiate_interface(
    const ttx_abstract* requirement) const -> ttx_interface {
  return requirement == ttx_type_requirement()
             ? ttx_interface_satisfied(
                   requirement, get_abi(), &operations.interface)
             : Abstract::negotiate_interface(requirement);
}
