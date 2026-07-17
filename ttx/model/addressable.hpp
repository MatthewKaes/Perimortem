// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "ttx/concept/abstract.hpp"

namespace Ttx::Model {

// Addressable is the narrow contract for a named semantic object that can be
// selected through an address-bearing context. A structured field, receiver,
// local, external symbol, interpreted endpoint, or runtime object may all be
// Addressable while publishing different richer contracts.
//
// get_name() supplies the addressable name and resolve() supplies the Abstract
// represented at that address. Structured Layouts borrow the actual
// Addressable objects, so documentation, attributes, defaults, ISA facts, and
// target storage remain on their real owners. Core TTX assigns no pointer
// width, offset, symbol spelling, calling convention, or storage policy.
class Addressable : public Concept::Abstract {
 public:
  using ContractOwner = Addressable;
  static constexpr Perimortem::System::Uuid contract_id{
    0x3d308ace3cf44051,
    0x9458c12c65d0d615,
  };

  auto implements(Perimortem::System::Uuid requested) const -> Bool override {
    return requested == contract_id || Abstract::implements(requested);
  }
};

}  // namespace Ttx::Model
