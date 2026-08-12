// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "ttx/concept/abstract.hpp"
#include "ttx/concept/invalid.hpp"
#include "ttx/concept/layout.hpp"
#include "ttx/model/layouts/value.hpp"

namespace Ttx::Model {

// Type is the narrow Abstract contract for semantic objects that can occupy a
// value or lowering position. It contributes target neutral recursive
// shape. Type itself does not require size, alignment, offsets, register
// selection, calling convention, documentation, aliases, or one universal
// child table. Terminal specializes Type with direct size and alignment facts.
// Composite Types derive those facts by walking their real Layout. Static and
// Name resolution remains the Abstract query on the durable Type itself. Type
// does not manufacture separate lexical scopes for static or receiver access.
//
// A host may reserve a stable Type before all of its facts are available. That
// object resolves to Invalid until its owner can answer the Type contract. No
// universal publication phase or Incomplete Layout is required. The host may
// later enrich the same nonmoving object, replace its enclosing system, or use
// another resolution policy without changing this interface.
//
// Once Type resolution succeeds, get_layout() returns its Layout shape. An
// atomic Type uses one Value leaf containing its exact identity. A structural
// or deliberately empty Type overrides that leaf with its real shape. A
// homogeneous Ranged layout can expose one repeated Type across a compact
// interval, as in Bytes[N], without allocating one semantic node per index.
class Type : public Concept::Abstract {
 public:
  TTX_CONTRACT(Type, Abstract, 0x94ee892e6e064e3b, 0x896512640a01b446);

  virtual constexpr auto get_layout() const -> const Concept::Layout& {
    return layout;
  }

 protected:
  constexpr Type() : layout(*this) {}

  // The terminal leaf borrows this exact semantic identity. Copying or moving
  // a Type would leave that leaf naming the original object, so every Type
  // remains a stable graph identity after construction.
  Type(const Type&) = delete;
  Type(Type&&) = delete;
  auto operator=(const Type&) -> Type& = delete;
  auto operator=(Type&&) -> Type& = delete;

 private:
  Layouts::Value layout;
};

}  // namespace Ttx::Model
