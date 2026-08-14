// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "ttx/concept/invalid.hpp"
#include "ttx/concept/layout.hpp"

namespace Ttx::Model {

// Pack is the Abstract contract for one produced value flow. It preserves the
// producer identity needed by linking, tools, and lowering while its Layout is
// the identity-free descriptor used to fit that flow into a required shape.
// A one-value Expression, an empty return, and a multi-value swizzle are all
// Packs without acquiring an aggregate Type.
//
// A staged Pack resolves to Invalid until its complete output Layout is known.
// Once resolve() returns the Pack itself, get_layout() is total and returns the
// same stable Layout for the Pack's remaining lifetime. An incomplete Pack must
// never expose an empty placeholder because an empty Layout is valid zero-value
// flow rather than an incomplete state.
class Pack : public Concept::Abstract {
 public:
  TTX_CONTRACT(Pack, Abstract);

  TTX_INVALID_CONTEXT;

  virtual constexpr auto get_layout() const -> const Concept::Layout& = 0;

  // Fitting remains directional: this Pack supplies the required target
  // Layout. The concrete output Layout retains the real source identities and
  // owns positional, named, ranged, or composed fitting behavior.
  virtual constexpr auto fits(const Concept::Layout& target) const -> Bool {
    return get_layout().fits(target);
  }
};

}  // namespace Ttx::Model
