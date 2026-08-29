// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "ttx/bootstrap/model/callable.hpp"

using namespace Perimortem::Core;

auto Ttx::Model::Callable::prove(Concept::Abstract& candidate)
    -> Option<Callable&> {
  ttx_callable_view view;
  return ttx_callable_prove(candidate.get_abi(), &view)
             ? Option<Callable&>(static_cast<Callable&>(candidate))
             : Option<Callable&>();
}

auto Ttx::Model::Callable::prove(const Concept::Abstract& candidate)
    -> Option<const Callable&> {
  ttx_callable_view view;
  return ttx_callable_prove(candidate.get_abi(), &view)
             ? Option<const Callable&>(static_cast<const Callable&>(candidate))
             : Option<const Callable&>();
}

static auto satisfied(const ttx_abstract*, const ttx_abstract*)
    -> ttx_interface_relation {
  return TTX_INTERFACE_SATISFIED;
}

static auto parameters(const ttx_abstract* abstract) -> const ttx_layout* {
  const auto& selected = static_cast<const Ttx::Model::Callable&>(
      Ttx::Concept::Abstract::from_abi(abstract));
  return selected.get_parameters().get_abi();
}

static auto results(const ttx_abstract* abstract) -> const ttx_layout* {
  const auto& selected = static_cast<const Ttx::Model::Callable&>(
      Ttx::Concept::Abstract::from_abi(abstract));
  return selected.get_results().get_abi();
}

static const ttx_callable_operations operations = {
  .interface = {.negotiate = satisfied},
  .parameters = parameters,
  .results = results,
};

auto Ttx::Model::Callable::negotiate_interface(
    const ttx_abstract* requirement) const -> ttx_interface {
  return requirement == ttx_callable_requirement()
             ? ttx_interface_satisfied(
                   requirement, get_abi(), &operations.interface)
             : Abstract::negotiate_interface(requirement);
}
