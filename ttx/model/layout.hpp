// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "ttx/abstraction/abstract.hpp"

namespace Ttx::Model {

// Layout is the common fitting contract for an ordered group of Abstracts.
// It does not copy names, Types, documentation, attributes, defaults, or target
// storage facts out of those objects. A consumer asks only for order, the real
// Abstract at an index, and whether one layout fits another.
//
// Fluid, Named, and Structured express their different fitting rules through
// inheritance rather than a tag on one record. Layout therefore has no
// incomplete state. An unfinished Type or host object resolves to Invalid;
// once resolution succeeds its concrete Layout contract is available.
class Layout {
 public:
  virtual ~Layout() = default;

  virtual auto get_size() const -> Count = 0;

  // The caller proves index is in range. Implementations borrow only real
  // Abstracts. An unresolved slot points to Invalid rather than nullptr.
  virtual auto get_abstract(Count index) const
      -> const Abstraction::Abstract& = 0;

  // Fitting is directional and owned by the source layout contract.
  virtual auto fits(const Layout& target) const -> Bool = 0;

  auto is_empty() const -> Bool { return get_size() == 0; }
};

}  // namespace Ttx::Model
