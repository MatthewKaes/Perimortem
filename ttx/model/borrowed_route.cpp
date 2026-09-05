// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "ttx/model/borrowed_route.hpp"

#include <cstddef>
#include <cstdlib>

using namespace Ttx;

auto BorrowedRoute::name() const -> ttx_borrowed_bytes {
  return bytes;
}

void BorrowedRoute::route(ttx_abstract self, ttx_route_result result) const {
  (void)self;
  binding.operations = {
    .header =
        {
          .size = sizeof(ttx_route_ops),
          .abi_major = TTX_ABI_MAJOR,
          .abi_minor = TTX_ABI_MINOR,
        },
    .candidate = candidate,
    .bytes = get_bytes,
  };
  result.operations->resolved(
      result, {
                .operations = &binding.operations,
                .self = reinterpret_cast<ttx_route_self*>(
                    const_cast<BorrowedRoute*>(this)),
              });
}

auto BorrowedRoute::negotiate(ttx_abstract requirement) const
    -> ttx_interface_relation {
  return ttx_abstract_same(requirement, ttx_route_requirement())
             ? TTX_INTERFACE_SATISFIED
             : Constant::negotiate(requirement);
}

auto BorrowedRoute::select(ttx_route self) -> const BorrowedRoute& {
  if (self.operations == nullptr || self.self == nullptr) {
    std::abort();
  }
  const auto& selected = *reinterpret_cast<const BorrowedRoute*>(self.self);
  if (&selected.binding.operations != self.operations) {
    std::abort();
  }
  return selected;
}

auto TTX_CALL BorrowedRoute::candidate(ttx_route self) -> ttx_abstract {
  return select(self).get_abi();
}

auto TTX_CALL BorrowedRoute::get_bytes(ttx_route self) -> ttx_borrowed_bytes {
  return select(self).bytes;
}
