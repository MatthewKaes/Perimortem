// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "ttx/concept/reference.hpp"
#include "ttx/model/exports.hpp"

namespace Tetrodotoxin::Model::Package {

// Resolved is Tetrodotoxin's public reflection and execution boundary. A
// consumer resolves the same exported Abstract graph whether the Package was
// interpreted from Sources or restored from a compiled buffer. Exports
// is the shared visibility contract. Package adds exact dependency closure and
// a canonical definition table for direct archive linkage.
//
// Package deliberately omits Source access. Source availability is the
// narrower Interpreted capability, proven through Abstract::is(). A
// Package is anonymous: repositories, manifests, filesystems, and Dependency
// bindings own every projected name used to locate one.
class Resolved : public Ttx::Model::Exports {
 public:
  using ContractOwner = Resolved;
  static constexpr Perimortem::System::Uuid contract_id{
    0xf1ad6c89330a46b8,
    0xa77ef9d0ff5d4ad2,
  };

  constexpr auto implements(Perimortem::System::Uuid requested) const
      -> Bool override {
    return requested == contract_id ||
           Ttx::Model::Exports::implements(requested);
  }

  constexpr auto get_name() const -> Perimortem::Core::View::Bytes override {
    return {};
  }

  virtual constexpr auto get_dependencies() const
      -> Perimortem::Core::View::Vector<Ttx::Concept::Reference<Resolved>> = 0;

  // Every package assigns a stable local ID to each definition in its complete
  // semantic graph. Archive edges can consequently name an external object as
  // `(dependency slot, definition ID)` without recursively searching exports.
  // Definitions need not be public. Exports remains the visibility boundary.
  virtual constexpr auto get_definition_count() const -> Count = 0;
  virtual auto get_definition(Count id) const
      -> const Ttx::Concept::Abstract& = 0;
  virtual auto get_definition_id(const Ttx::Concept::Abstract& definition) const
      -> Count = 0;
};

}  // namespace Tetrodotoxin::Model::Package
