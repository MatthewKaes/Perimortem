// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "ttx/concept/abstract.hpp"

namespace Ttx::Model {

// Exports is the narrow contract for an Abstract that publishes an ordered
// named definition surface. Source dependencies, package dependencies, nested
// groups, restored packages, reflection, and tooling can all consume this same
// contract without knowing which Dialect or storage mechanism produced it.
//
// Every index selects the real exported Abstract edge in authored publication
// order. Its name is nonempty and unique within this surface. Resolving that
// name directly in this context returns the same edge. An exported Alias
// therefore remains visible as the local named edge while its own resolve()
// redirects to canonical identity. An index outside the surface returns
// Invalid.
//
// Direct lookup is closed over the enumerated surface. A name resolves from an
// Exports context exactly when one indexed edge owns that name. Private roots
// and imported contexts remain unreachable through this boundary. Nested
// lookup begins only after selecting one exported edge, which keeps dependency
// resolution local instead of searching the producer's complete source graph.
//
// Exports owns no collection implementation, mutation API, visibility Kind, or
// route grammar. A concrete owner retains the stable references, validates
// publication, and implements resolve_context(). The contract only makes the
// durable public graph enumerable as well as resolvable.
class Exports : public Concept::Abstract {
 public:
  using ContractOwner = Exports;
  static constexpr Perimortem::System::Uuid contract_id{
    0x607ebb5ff2e846fc,
    0xac5f553e6df25102,
  };

  constexpr auto implements(Perimortem::System::Uuid requested) const
      -> Bool override {
    return requested == contract_id || Abstract::implements(requested);
  }

  virtual constexpr auto get_export_count() const -> Count = 0;
  virtual constexpr auto get_export(Count index) const
      -> const Concept::Abstract& = 0;
};

}  // namespace Ttx::Model
