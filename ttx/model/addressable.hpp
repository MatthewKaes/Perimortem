// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "ttx/model/type.hpp"

namespace Ttx::Model {

// Addressable is the narrow contract for a named address to typed data. A
// structured field, receiver, local, external symbol, interpreted endpoint, or
// runtime object may all be Addressable while publishing different richer
// contracts. Capabilities related to the Addressable are granted by Dialects
// specific enrichments, so things like `Writability` are a language construct.
//
class Addressable : public Concept::Abstract {
 public:
  using ClassCatagory = Addressable;
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

  virtual constexpr auto get_type() const -> const Type& = 0;
};

}  // namespace Ttx::Model
