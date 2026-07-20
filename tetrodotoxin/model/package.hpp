// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "ttx/concept/reference.hpp"
#include "ttx/model/exports.hpp"

namespace Tetrodotoxin::Model {

// Package is Tetrodotoxin's public reflection and execution boundary. A
// consumer resolves the same exported Abstract graph whether the Package was
// interpreted from Sources or restored from a compiled Puffer Buffer. Exports
// is the shared graph contract. Package adds only package dependency closure.
//
// Package deliberately omits Source access. Source availability is the
// narrower Packages::Interpreted capability, proven through Abstract::is(). A
// Package is anonymous: repositories, manifests, filesystems, and Dependency
// bindings own every projected name used to locate one.
class Package : public Ttx::Model::Exports {
 public:
  using ContractOwner = Package;
  static constexpr Perimortem::System::Uuid contract_id{
    0xf1ad6c89330a46b8,
    0xa77ef9d0ff5d4ad2,
  };

  constexpr auto implements(Perimortem::System::Uuid requested) const
      -> Bool override {
    return requested == contract_id ||
           Ttx::Model::Exports::implements(requested);
  }

  constexpr auto get_name() const -> Perimortem::Core::View::Bytes final {
    return {};
  }

  virtual constexpr auto get_dependencies() const
      -> Perimortem::Core::View::Vector<Ttx::Concept::Reference<Package>> = 0;
};

}  // namespace Tetrodotoxin::Model
