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
// Model::Layouts::Fluid, Named, Structured, Ranged, and Composite express
// different fitting rules through inheritance rather than a tag on one record.
// Layout therefore has no incomplete state. An unfinished Type or host object
// resolves to Invalid. Once resolution succeeds its concrete Layout contract
// is available.
class Layout {
 public:
  constexpr virtual ~Layout() = default;

  virtual constexpr auto get_size() const -> Count = 0;

  // Implementations borrow only real Abstracts through a Reference that can
  // never be null. An index outside the Layout returns Invalid without
  // imposing a precondition or storing a nullable pointer.
  virtual constexpr auto get_abstract(Count index) const -> const Abstract& = 0;

  // Fitting is directional and owned by the source Layout contract. Ordinary
  // fitting requires the complete source and target to have the same size.
  constexpr auto fits(const Layout& target) const -> Bool {
    return get_size() == target.get_size() && fits_at(target, 0);
  }

  // Fits this complete source Layout into the target segment beginning at the
  // provided offset. Composite uses this operation to preserve each child's
  // fitting rules without flattening either Layout into copied entries.
  virtual constexpr auto fits_at(const Layout& target, Count target_offset)
      const -> Bool = 0;

  // Returns the source Abstract that supplies one target slot. A failed fit,
  // invalid target index, or missing mapping returns Invalid. This exposes the
  // ordering evidence found during fitting without allocating a mapping or
  // forcing every consumer to repeat Named matching.
  constexpr auto get_fitted(const Layout& target, Count target_index) const
      -> const Abstract& {
    if (get_size() != target.get_size()) {
      return Invalid::get_invalid();
    }

    return get_fitted_at(target, 0, target_index);
  }

  // Returns the source Abstract that supplies one index in a target segment.
  // The target index is local to this source Layout. Composite translates that
  // index and target offset before delegating to the selected child.
  virtual constexpr auto get_fitted_at(
      const Layout& target,
      Count target_offset,
      Count target_index) const -> const Abstract& = 0;

  constexpr auto is_empty() const -> Bool { return get_size() == 0; }

 protected:
  constexpr auto has_target_segment(const Layout& target, Count target_offset)
      const -> Bool {
    return target_offset <= target.get_size() &&
           get_size() <= target.get_size() - target_offset;
  }
};

}  // namespace Ttx::Concept
