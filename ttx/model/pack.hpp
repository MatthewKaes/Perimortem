// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

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
  TTX_CONTRACT(Pack, Abstract);

  TTX_INVALID_CONTEXT;

  virtual constexpr auto get_layout() const -> const Concept::Layout& = 0;

  // Fitting remains directional: this Pack supplies the required target
  // Layout. The concrete output Layout retains the real source identities and
  // owns positional, named, ranged, or composed fitting behavior. An Invalid
  // Pack never supplies even when its safe inspection Layout is empty.
  virtual constexpr auto fits(const Concept::Layout& target) const -> Bool {
    return &resolve() == this && get_layout().fits(target);
  }
};

}  // namespace Ttx::Model
