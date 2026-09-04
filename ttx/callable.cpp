// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "ttx/callable.hpp"

#include <cstddef>
#include <cstdlib>

using namespace Ttx;

Callable::Callable() : Callable(ttx_authority_create(), 1) {}

Callable::Callable(uint64_t authority, uint64_t value)
    : Abstract(authority, value),
      binding({
        .operations =
            {
              .header =
                  {
                    .size = sizeof(ttx_callable_ops),
                    .abi_major = TTX_ABI_MAJOR,
                    .abi_minor = TTX_ABI_MINOR,
                  },
              .candidate = candidate,
              .parameters = get_parameters,
              .results = get_results,
            },
        .owner = this,
      }) {}

void Callable::callable(ttx_abstract self, ttx_callable_result result) const {
  const ttx_callable view = {
    .operations = &binding.operations,
    .owner = self.owner,
    .value = self.value,
  };
  result.operations->resolved(result, view);
}

auto Callable::negotiate(ttx_abstract requirement) const
    -> ttx_interface_relation {
  return ttx_abstract_same(requirement, ttx_callable_requirement())
             ? TTX_INTERFACE_SATISFIED
             : Abstract::negotiate(requirement);
}

auto Callable::select(ttx_callable self) -> const Callable& {
  if (self.operations == nullptr) {
    std::abort();
  }
  static_assert(offsetof(Binding, operations) == 0);
  const auto& selected = *reinterpret_cast<const Binding*>(self.operations);
  if (selected.owner == nullptr) {
    std::abort();
  }
  const ttx_abstract candidate = selected.owner->get_abi();
  if (candidate.owner != self.owner || candidate.value != self.value) {
    std::abort();
  }
  return *selected.owner;
}

auto TTX_CALL Callable::candidate(ttx_callable self) -> ttx_abstract {
  return select(self).get_abi();
}

auto TTX_CALL Callable::get_parameters(ttx_callable self) -> ttx_layout {
  return select(self).parameters();
}

auto TTX_CALL Callable::get_results(ttx_callable self) -> ttx_layout {
  return select(self).results();
}
