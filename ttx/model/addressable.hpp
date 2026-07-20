// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "ttx/concept/abstract.hpp"

namespace Ttx::Model {

// Addressable is the narrow contract for a named address to typed data. A
// structured field, receiver, local, external symbol, interpreted endpoint, or
// runtime object may all be Addressable while publishing different richer
// contracts. Write capability is the narrower Addressables::Writable contract,
// not a boolean or Kind carried directly on Addressable. This distinction is
// unrelated to a source `const` definition, whose initializer must instead
// materialize completely at compile time.
//
// get_name() supplies the addressable name and get_type() supplies its Type or
// Invalid. Address identity does not resolve away into that Type. Structured
// Layouts borrow the actual Addressable objects, so documentation, attributes,
// defaults, Dialect facts, and target storage remain on their real owners. Core TTX
// assigns no pointer width, offset, symbol spelling, calling convention, or
// storage policy.
class Addressable : public Concept::Abstract {
 public:
  using ContractOwner = Addressable;
  static constexpr Perimortem::System::Uuid contract_id{
    0x3d308ace3cf44051,
    0x9458c12c65d0d615,
  };

  constexpr auto implements(Perimortem::System::Uuid requested) const
      -> Bool override {
    return requested == contract_id || Abstract::implements(requested);
  }

  constexpr auto resolve_context(Perimortem::Core::View::Bytes route) const
      -> const Concept::Abstract& override {
    return get_type().resolve().resolve_context(route);
  }

  virtual constexpr auto get_type() const -> const Concept::Abstract& = 0;
};

}  // namespace Ttx::Model
