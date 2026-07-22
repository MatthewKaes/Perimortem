// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "tetrodotoxin/model/packages/compiled.hpp"
#include "tetrodotoxin/model/source.hpp"

namespace Tetrodotoxin::Model::Packages {

// Interpreted is the optional Package capability for a package whose resolved
// Sources remain available. Formatters, editors, and incremental compilation
// may prove this contract and inspect those Sources. Ordinary Package consumers
// remain independent of whether source survived construction.
//
// Interpreted is a contract rather than a concrete storage class so embedded
// runtimes and alternative source systems can provide their own implementation.
class Interpreted : public Compiled {
 public:
  using ContractOwner = Interpreted;
  static constexpr Perimortem::System::Uuid contract_id{
    0x79335eb83bb041df,
    0xb99416db24775b5e,
  };

  auto implements(Perimortem::System::Uuid requested) const -> Bool override {
    return requested == contract_id || Compiled::implements(requested);
  }

  virtual auto get_sources() const -> Perimortem::Core::View::Vector<
      Ttx::Concept::Reference<Model::Source>> = 0;
};

}  // namespace Tetrodotoxin::Model::Packages
