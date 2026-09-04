// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "ttx/finite_extent.hpp"

#include <cstddef>
#include <cstdlib>

using namespace Ttx;

Extent::Extent(uint64_t cardinality)
    : extent_binding({
        .operations =
            {
              .header =
                  {
                    .size = sizeof(ttx_finite_extent_ops),
                    .abi_major = TTX_ABI_MAJOR,
                    .abi_minor = TTX_ABI_MINOR,
                  },
              .candidate = candidate,
              .cardinality = Extent::cardinality,
            },
        .owner = this,
      }),
      count(cardinality) {}

auto Extent::name() const -> ttx_borrowed_bytes {
  static const uint8_t value[] = "Extent";
  return {value, sizeof(value) - 1};
}

void Extent::finite_extent(ttx_abstract self, ttx_finite_extent_result result)
    const {
  result.operations->resolved(
      result, {
                .operations = &extent_binding.operations,
                .owner = self.owner,
                .value = self.value,
              });
}

auto Extent::negotiate(ttx_abstract requirement) const
    -> ttx_interface_relation {
  if (ttx_abstract_same(requirement, get_abi())) {
    return TTX_INTERFACE_EQUIVALENT;
  }
  if (ttx_abstract_same(requirement, ttx_constant_requirement()) ||
      ttx_abstract_same(requirement, ttx_finite_extent_requirement())) {
    return TTX_INTERFACE_SATISFIED;
  }
  return TTX_INTERFACE_REJECTED;
}

auto Extent::select(ttx_finite_extent self) -> const Extent& {
  if (self.operations == nullptr) {
    std::abort();
  }
  static_assert(offsetof(Binding, operations) == 0);
  const auto& binding = *reinterpret_cast<const Binding*>(self.operations);
  if (binding.owner == nullptr ||
      binding.owner->get_abi().owner != self.owner ||
      binding.owner->get_abi().value != self.value) {
    std::abort();
  }
  return *binding.owner;
}

auto TTX_CALL Extent::candidate(ttx_finite_extent self) -> ttx_abstract {
  return select(self).get_abi();
}

auto TTX_CALL Extent::cardinality(ttx_finite_extent self) -> uint64_t {
  return select(self).count;
}
