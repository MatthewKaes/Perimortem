// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/option.hpp"

#include "perimortem/utility/result.hpp"

#include "ttx/concept/abstract.hpp"

namespace Ttx::Concept {

// Layout describes the shape promised by a Type or Callable and the shape
// supplied by a Pack. It borrows the real semantic identities in that shape, so
// fitting can preserve both order and source ownership without creating another
// graph object.
//
// Consumers can inspect an entry, borrow its slot name, test fitting, and find
// the source object that supplies a target slot. Documentation, defaults, and
// physical storage remain with the owners that understand them.
//
// Value, Fluid, Named, Ranged, and Composite Layouts each express their fitting
// rule through the same contract. Once a semantic owner resolves successfully,
// its concrete Layout is complete and ready to share.
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
  // slot names. Composite delegates it to the child that owns the source edge.
  // The indices belong to the complete source and target Layouts.
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
