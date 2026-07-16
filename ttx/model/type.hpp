// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "ttx/abstraction/abstract.hpp"
#include "ttx/model/layouts/structured.hpp"

namespace Ttx::Model {

// Type is the narrow Abstract contract for semantic objects that can occupy a
// value or lowering position. It contributes target-independent recursive
// shape. Type does not own size, alignment, offsets, register selection,
// calling convention, documentation, aliases, or one universal child table.
// Derived Types resolve their own static and Self contexts using the ordinary
// Abstract route query and may optimize those contexts independently.
//
// A host may reserve a stable Type before all of its facts are available. That
// object resolves to Invalid until its owner can answer the Type contract; no
// universal publication phase or Incomplete Layout is required. The host may
// later enrich the same nonmoving object, replace its enclosing system, or use
// another resolution policy without changing this interface.
//
// Once Type resolution succeeds, get_layout() returns its Structured shape.
// The structure contains real Addressable objects whose own resolution yields
// the child Types needed by fitting and lowering.
class Type : public Abstraction::Abstract {
 public:
  virtual auto get_layout() const -> const Layouts::Structured& = 0;
};

}  // namespace Ttx::Model
