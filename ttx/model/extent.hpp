// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "ttx/concept/constant.hpp"

namespace Ttx {

// Ranged needs a cardinality before it can expose producer occurrences through
// Enumerable. Reusing a language integer would make every host understand that
// language's numeric model just to count positions. Extent narrows the
// exchange to one immutable nonnegative count after the candidate has proved
// the FiniteExtent requirement. Arithmetic and integer representation remain
// outside this contract.
class Extent final : public Constant {
 public:
  constexpr explicit Extent(uint64_t cardinality)
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
        }),
        count(cardinality) {}

  auto name() const -> ttx_borrowed_bytes override;
  void finite_extent(ttx_abstract self, ttx_finite_extent_result result)
      const override;

 protected:
  auto negotiate(ttx_abstract requirement) const
      -> ttx_interface_relation override;

 private:
  struct Binding {
    ttx_finite_extent_ops operations;
  };

  static auto select(ttx_finite_extent self) -> const Extent&;
  static auto TTX_CALL candidate(ttx_finite_extent self) -> ttx_abstract;
  static auto TTX_CALL cardinality(ttx_finite_extent self) -> uint64_t;

  const Binding extent_binding;
  const uint64_t count;
};

}  // namespace Ttx
