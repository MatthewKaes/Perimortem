// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "ttx/route.hpp"

#include <cstddef>
#include <cstdlib>
#include <utility>

using namespace Ttx;

Route::Route(std::vector<uint8_t> bytes)
    : Route(std::move(bytes), ttx_authority_create(), 1) {}

Route::Route(std::vector<uint8_t> bytes, uint64_t authority, uint64_t value)
    : Abstract(authority, value),
      binding({
        .operations =
            {
              .header =
                  {
                    .size = sizeof(ttx_route_ops),
                    .abi_major = TTX_ABI_MAJOR,
                    .abi_minor = TTX_ABI_MINOR,
                  },
              .candidate = candidate,
              .bytes = get_bytes,
            },
        .owner = this,
      }),
      bytes(std::move(bytes)) {}

auto Route::name() const -> ttx_borrowed_bytes {
  return {
    .data = bytes.data(),
    .size = bytes.size(),
  };
}

void Route::route(ttx_abstract self, ttx_route_result result) const {
  const ttx_route view = {
    .operations = &binding.operations,
    .owner = self.owner,
    .value = self.value,
  };
  result.operations->resolved(result, view);
}

auto Route::negotiate(ttx_abstract requirement) const
    -> ttx_interface_relation {
  if (ttx_abstract_same(requirement, ttx_route_requirement()) ||
      ttx_abstract_same(requirement, ttx_constant_requirement())) {
    return TTX_INTERFACE_SATISFIED;
  }
  return Abstract::negotiate(requirement);
}

auto Route::select(ttx_route self) -> const Route& {
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

auto TTX_CALL Route::candidate(ttx_route self) -> ttx_abstract {
  return select(self).get_abi();
}

auto TTX_CALL Route::get_bytes(ttx_route self) -> ttx_borrowed_bytes {
  return select(self).name();
}
