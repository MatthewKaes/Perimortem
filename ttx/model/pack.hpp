// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/option.hpp"

#include "ttx/concept/invalid.hpp"
#include "ttx/concept/layout.hpp"

namespace Ttx::Model {

// Pack is the Abstract contract for one produced value flow. It preserves the
// producer identity needed by linking, tools, and lowering while its Layout is
// the identity free descriptor used to fit that flow into a required shape.
// A one value Expression, an empty return, and a multiple value swizzle are all
// Packs without acquiring an aggregate Type.
//
// Layout inspection is total even while a staged Pack resolves Invalid. A
// missing or nonvalue output may expose an empty Layout safely, but only a Pack
// that resolves to itself supplies that shape as completed zero value flow.
// Once resolution succeeds the Layout is stable for the Pack's lifetime.
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
