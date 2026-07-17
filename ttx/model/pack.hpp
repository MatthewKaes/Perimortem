// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "ttx/concept/abstract.hpp"
#include "ttx/concept/layout.hpp"

namespace Ttx::Model {

// Pack is the Abstract mechanism for one grouped value flow. It is neither one
// Expression nor a Type, and resolving it preserves Pack identity. Pack
// publishes the fundamental Concept::Layout contract without becoming the
// shape itself. Positional packs expose Fluid while named packs expose Named.
//
// Pack has no local name or child context. Its entries remain the real
// Abstracts selected by the source owner, allowing tools and lowering to
// inspect value provenance without inventing a tuple Type.
class Pack : public Concept::Abstract {
 public:
  using ContractOwner = Pack;
  static constexpr Perimortem::System::Uuid contract_id{
    0x5fa43e02745846de,
    0xb1d6fb6e8eade155,
  };

  auto implements(Perimortem::System::Uuid requested) const -> Bool override {
    return requested == contract_id || Abstract::implements(requested);
  }

  auto get_name() const -> Perimortem::Core::View::Bytes final;
  auto resolve_context(Perimortem::Core::View::Bytes route) const
      -> const Concept::Abstract& final;

  virtual auto get_layout() const -> const Concept::Layout& = 0;
};

}  // namespace Ttx::Model
