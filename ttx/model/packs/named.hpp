// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/vector.hpp"

#include "ttx/concept/reference.hpp"
#include "ttx/model/layouts/named.hpp"
#include "ttx/model/pack.hpp"

namespace Ttx::Model::Packs {

// Named is a Pack whose Layout retains the actual named Abstracts in authored
// carrier order. Source fields use Binding when the authored name is a new edge
// rather than a fact already owned by the value. Names select target fields
// during fitting but never erase order or create Addressable storage.
//
// Named Packs do not flatten. The source owner rejects attempts to mix source
// modes before construction. Named owns its fitting view without acquiring
// Layout as a public semantic contract.
class Named final : public Pack {
 public:
  using ContractOwner = Named;
  static constexpr Perimortem::System::Uuid contract_id{
    0xdba65b79665c4fba,
    0xa4abde5dc37d3985,
  };

  Named(
      Perimortem::Core::View::Vector<Concept::Reference<Concept::Abstract>>
          bindings);

  auto implements(Perimortem::System::Uuid requested) const -> Bool override;
  auto get_layout() const -> const Layouts::Named& override;

 private:
  Layouts::Named layout;
};

}  // namespace Ttx::Model::Packs
