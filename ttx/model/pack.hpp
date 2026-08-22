// Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/option.hpp"

#include "ttx/concept/invalid.hpp"
#include "ttx/concept/layout.hpp"

namespace Ttx::Model {

// Pack carries one produced value flow and remembers the semantic object that
// created each visible output. Its Layout describes how that flow can fit a
// receiving contract. One value, an empty return, a named argument group, and a
// multiple value swizzle can all remain Packs until a language chooses a Type
// that materializes them.
//
// Layout inspection remains available while staged flow is incomplete. Once a
// Pack resolves successfully, its output Layout stays stable for the lifetime
// of that graph.
class Pack : public Concept::Abstract {
 public:
  // Produced identifies the real Pack that computes one visible output slot.
  // Composed Packs translate their public index to the producer's local index
  // instead of making a Layout entry double as an executable edge. The query
  // value borrows the completed graph and creates no retained identity.
  struct Produced {
    const Pack& producer;
    Count local_index;
  };

  TTX_CONTRACT(Pack, Abstract);

  TTX_INVALID_CONTEXT;

  virtual constexpr auto get_layout() const -> const Concept::Layout& = 0;

  // Returns the exact producer for one output position. Absence means the
  // index is outside this Pack's complete output. A scalar Pack returns itself
  // at local index zero. Grouping and selection owners preserve their real
  // child producer instead.
  virtual auto get_produced(Count index) const
      -> Perimortem::Core::Option<Produced> = 0;

  // Fitting remains directional: this Pack supplies the required target
  // Layout. The concrete output Layout retains the real source identities and
  // owns positional, named, ranged, or composed fitting behavior. An Invalid
  // Pack never supplies even when its safe inspection Layout is empty.
  virtual constexpr auto fits(const Concept::Layout& target) const -> Bool {
    return &resolve() == this && get_layout().fits(target);
  }
};

}  // namespace Ttx::Model
