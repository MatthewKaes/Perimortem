// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "ttx/concept/invalid.hpp"

namespace Ttx::Concept {

// Layout is the fundamental fitting contract for an ordered group of
// Abstracts. It is not an Abstract and describes shape without acquiring
// semantic identity, resolution, or ownership of the values that expose it.
//
// Layout does not copy names, Types, documentation, attributes, defaults, or
// target storage facts out of its entries. A consumer asks only for order, the
// real Abstract at an index, whether one layout fits another, and the source
// object that supplies each target slot. Failed queries return Invalid so
// source owners can diagnose authored shape errors without trapping.
//
// Model::Layouts::Fluid, Named, and Structured express different fitting rules
// through inheritance rather than a tag on one record. Layout therefore has no
// incomplete state. An unfinished Type or host object resolves to Invalid.
// Once resolution succeeds its concrete Layout contract is available.
class Layout {
 public:
  constexpr virtual ~Layout() = default;

  virtual constexpr auto get_size() const -> Count = 0;

  // Implementations borrow only real Abstracts through a non-null Reference.
  // An out-of-range index returns Invalid rather than imposing a precondition
  // or storing a nullable pointer.
  virtual constexpr auto get_abstract(Count index) const -> const Abstract& = 0;

  // Fitting is directional and owned by the source layout contract.
  virtual constexpr auto fits(const Layout& target) const -> Bool = 0;

  // Returns the source Abstract that supplies one target slot. A failed fit,
  // invalid target index, or missing mapping returns Invalid. This exposes the
  // ordering evidence found during fitting without allocating a mapping or
  // forcing every consumer to repeat Named matching.
  virtual constexpr auto get_fitted(const Layout& target, Count target_index)
      const -> const Abstract& = 0;

  constexpr auto is_empty() const -> Bool { return get_size() == 0; }
};

}  // namespace Ttx::Concept
