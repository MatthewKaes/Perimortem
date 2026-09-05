// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "ttx/model/route.hpp"

#include <cstddef>
#include <cstdlib>
#include <utility>

using namespace Ttx;

Route::Route(std::vector<uint8_t> bytes)
    : binding({
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
      }),
      bytes(std::move(bytes)) {}

auto Route::name() const -> ttx_borrowed_bytes {
  return {
    .data = bytes.data(),
    .size = bytes.size(),
  };
}

void Route::route(ttx_abstract self, ttx_route_result result) const {
  (void)self;
  const ttx_route view = {
    .operations = &binding.operations,
    .self = reinterpret_cast<ttx_route_self*>(const_cast<Route*>(this)),
  };
  result.operations->resolved(result, view);
}

auto Route::negotiate(ttx_abstract requirement) const
    -> ttx_interface_relation {
  if (ttx_abstract_same(requirement, ttx_route_requirement())) {
    return TTX_INTERFACE_SATISFIED;
  }
  return Constant::negotiate(requirement);
}

auto Route::select(ttx_route self) -> const Route& {
  if (self.operations == nullptr || self.self == nullptr) {
    std::abort();
  }
  const auto& selected = *reinterpret_cast<const Route*>(self.self);
  if (&selected.binding.operations != self.operations) {
    std::abort();
  }
  return selected;
}

auto TTX_CALL Route::candidate(ttx_route self) -> ttx_abstract {
  return select(self).get_abi();
}

auto TTX_CALL Route::get_bytes(ttx_route self) -> ttx_borrowed_bytes {
  return select(self).name();
}
