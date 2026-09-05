// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/null_terminated.hpp"

#include "ttx/concept/abstract.hpp"

namespace Ttx::Concept {

// Unknown preserves an indeterminate answer without predicting whether another
// observation will produce a value, prove absence, or remain indeterminate.
// Every query returns that same open answer, so a consumer can propagate it but
// cannot extract evidence that would let work proceed as though it had settled.
class Unknown : public Abstract {
 public:
  static auto get_unknown() -> const Unknown&;
  Unknown(const Unknown&) = delete;
  auto operator=(const Unknown&) -> Unknown& = delete;

  TTX_NAME("Unknown"_view);
  TTX_EMPTY_DOCUMENTATION();

  auto resolve_concept(ttx_borrowed_bytes) const -> ttx_abstract override {
    return get_abi();
  }

  void domain(ttx_abstract self, ttx_domain_result result) const override;
  void callable(ttx_abstract self, ttx_callable_result result) const override;
  void route(ttx_abstract self, ttx_route_result result) const override;
  void finite_extent(ttx_abstract self, ttx_finite_extent_result result)
      const override;
  void bytes(ttx_abstract self, ttx_bytes_result result) const override;

 private:
  Unknown() = default;

  auto negotiate(ttx_abstract requirement) const
      -> ttx_interface_relation override;
};

}  // namespace Ttx::Concept
