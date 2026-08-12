// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "ttx/model/type.hpp"

namespace Ttx::Model {

// Addressable is the narrow contract for a named address to typed data whose
// exact Type exposes at least one Layout entry. An empty Type remains a valid
// semantic domain but supplies no value whose stable address can be named. A
// structured field, receiver, local, external symbol, interpreted endpoint, or
// runtime object may all be Addressable while publishing different richer
// contracts. Capabilities related to the Addressable are granted by Dialects
// specific enrichments, so things like `Writability` are a language construct.
//
class Addressable : public Concept::Abstract {
 public:
  TTX_CONTRACT(Addressable, Abstract, 0x3d308ace3cf44051, 0x9458c12c65d0d615);

  constexpr auto resolve_context(Perimortem::Core::View::Bytes route) const
      -> const Concept::Abstract& override {
    return get_type().resolve().resolve_context(route);
  }

  virtual constexpr auto get_type() const -> const Type& = 0;
};

}  // namespace Ttx::Model
