// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "ttx/abstract.hpp"

namespace Ttx {

// Extent is the immutable support fact required by Ranged. It
// exposes one nonnegative cardinality after exact Interface negotiation without
// making integers or numeric storage part of the shared semantic vocabulary.
class Extent final : public Abstract {
 public:
  explicit Extent(uint64_t cardinality);

  auto name() const -> ttx_borrowed_bytes override;
  void finite_extent(ttx_abstract self, ttx_finite_extent_result result)
      const override;

 protected:
  auto negotiate(ttx_abstract requirement) const
      -> ttx_interface_relation override;

 private:
  struct Binding {
    ttx_finite_extent_ops operations;
    const Extent* owner;
  };

  static auto select(ttx_finite_extent self) -> const Extent&;
  static auto TTX_CALL candidate(ttx_finite_extent self) -> ttx_abstract;
  static auto TTX_CALL cardinality(ttx_finite_extent self) -> uint64_t;

  const Binding extent_binding;
  const uint64_t count;
};

}  // namespace Ttx
