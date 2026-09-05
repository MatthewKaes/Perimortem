// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "ttx/model/extent.hpp"

#include <cstddef>
#include <cstdlib>

using namespace Ttx;

auto Extent::name() const -> ttx_borrowed_bytes {
  static const uint8_t value[] = "Extent";
  return {value, sizeof(value) - 1};
}

void Extent::finite_extent(ttx_abstract self, ttx_finite_extent_result result)
    const {
  (void)self;
  result.operations->resolved(
      result, {
                .operations = &extent_binding.operations,
                .self = reinterpret_cast<ttx_finite_extent_self*>(
                    const_cast<Extent*>(this)),
              });
}

auto Extent::negotiate(ttx_abstract requirement) const
    -> ttx_interface_relation {
  if (ttx_abstract_same(requirement, get_abi())) {
    return TTX_INTERFACE_EQUIVALENT;
  }
  if (ttx_abstract_same(requirement, ttx_finite_extent_requirement())) {
    return TTX_INTERFACE_SATISFIED;
  }
  return Constant::negotiate(requirement);
}

auto Extent::select(ttx_finite_extent self) -> const Extent& {
  if (self.operations == nullptr || self.self == nullptr) {
    std::abort();
  }
  const auto& selected = *reinterpret_cast<const Extent*>(self.self);
  if (&selected.extent_binding.operations != self.operations) {
    std::abort();
  }
  return selected;
}

auto TTX_CALL Extent::candidate(ttx_finite_extent self) -> ttx_abstract {
  return select(self).get_abi();
}

auto TTX_CALL Extent::cardinality(ttx_finite_extent self) -> uint64_t {
  return select(self).count;
}
