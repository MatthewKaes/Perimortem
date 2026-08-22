// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/option.hpp"

#include "perimortem/utility/result.hpp"

#include "ttx/concept/abstract.hpp"

namespace Ttx::Concept {

// Layout is the identity-free descriptor for one promised value shape. Types
// and Callables expose required Layouts while Packs expose the Layout of values
// they supply. Layout is not an Abstract and acquires no producer identity,
// resolution, or ownership of the objects it describes.
//
// Layout does not copy Types, documentation, attributes, defaults, or target
// storage facts out of its entries. It may borrow names for its slots without
// renaming the source Abstracts. A consumer asks only for order, the real
// Abstract at an index, whether one Layout fits another, and the source object
// that supplies each target slot. Indexing absence and fitting errors remain
// ordinary query results rather than semantic Invalid graph edges.
//
// Model::Layouts::Value, Fluid, Named, Ranged, and Composite express different
// fitting rules through inheritance rather than a tag on one record. Layout
// therefore has no incomplete state. An unfinished Type, Pack, or host object
// resolves to Invalid. Once resolution succeeds its concrete Layout contract is
// available.
class Layout {
 public:
  enum class Errors : U8 {
    IndexOutOfBounds,
    SizeMismatch,
    IncompatibleFit,
  };

  constexpr virtual ~Layout() = default;

  virtual constexpr auto get_size() const -> Count = 0;

  // Implementations borrow only real Abstracts through a Reference that can
  // never be null. An index outside the Layout returns None.
  virtual constexpr auto get_abstract(Count index) const
      -> Perimortem::Core::Option<const Abstract&> = 0;

  // A named Layout may attach one name to each source slot without
  // changing the Abstract that supplies that slot. Positional Layouts return
  // absence. Keeping the query on Layout lets tools and fitting observe the
  // named shape without manufacturing Alias identities for renamed flow.
  virtual constexpr auto get_name(Count index) const
      -> Perimortem::Core::Option<Perimortem::Core::View::Bytes> {
    return {};
  }

  // Tests one source slot against one target slot while preserving the source
  // Layout's concrete fitting rules. Named uses this primitive after matching
  // slot names; Composite delegates it to the child that owns the source
  // edge. The indices belong to the complete source and target Layouts.
  virtual constexpr auto fits_entry(
      const Layout& target,
      Count source_index,
      Count target_index) const -> Bool = 0;

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

  // Returns the source Abstract that supplies one target slot. Every query
  // selects either that reference or one Errors value and never returns the
  // Union's null state. This exposes the ordering evidence found during fitting
  // without allocating a mapping or forcing every consumer to repeat Named
  // matching.
  constexpr auto get_fitted(const Layout& target, Count target_index) const
      -> Perimortem::Utility::Result<const Abstract&, Errors> {
    if (target_index >= get_size()) {
      return Errors::IndexOutOfBounds;
    }

    if (get_size() != target.get_size()) {
      return Errors::SizeMismatch;
    }

    return get_fitted_at(target, 0, target_index);
  }

  // Returns the source Abstract that supplies one index in a target segment.
  // The target index is local to this source Layout. Composite translates that
  // index and target offset before delegating to the selected child.
  virtual constexpr auto get_fitted_at(
      const Layout& target,
      Count target_offset,
      Count target_index) const
      -> Perimortem::Utility::Result<const Abstract&, Errors> = 0;

  constexpr auto is_empty() const -> Bool { return get_size() == 0; }

 protected:
  constexpr auto has_target_segment(const Layout& target, Count target_offset)
      const -> Bool {
    return target_offset <= target.get_size() &&
           get_size() <= target.get_size() - target_offset;
  }
};

}  // namespace Ttx::Concept
