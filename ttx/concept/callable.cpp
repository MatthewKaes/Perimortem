// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "ttx/concept/callable.hpp"

#include <cstddef>
#include <cstdlib>

using namespace Ttx;

void Callable::callable(ttx_abstract, ttx_callable_result result) const {
  const ttx_callable view = {
    .operations = &binding.operations,
    .self = reinterpret_cast<ttx_callable_self*>(
        const_cast<Callable*>(this)),
  };
  result.operations->resolved(result, view);
}

auto Callable::negotiate(ttx_abstract requirement) const
    -> ttx_interface_relation {
  return ttx_abstract_same(requirement, ttx_callable_requirement())
             ? TTX_INTERFACE_SATISFIED
             : Abstract::negotiate(requirement);
}

auto Callable::select_abi(ttx_callable self) -> const Callable& {
  if (self.operations == nullptr || self.self == nullptr) {
    std::abort();
  }
  const auto& selected = *reinterpret_cast<const Callable*>(self.self);
  if (&selected.binding.operations != self.operations) {
    std::abort();
  }
  return selected;
}

auto TTX_CALL Callable::candidate(ttx_callable self) -> ttx_abstract {
  return select_abi(self).get_abi();
}

auto TTX_CALL Callable::get_parameters(ttx_callable self) -> ttx_layout {
  return select_abi(self).parameters();
}

auto TTX_CALL Callable::get_results(ttx_callable self) -> ttx_layout {
  return select_abi(self).results();
}
