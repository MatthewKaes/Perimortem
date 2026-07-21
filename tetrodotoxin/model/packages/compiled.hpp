// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "tetrodotoxin/model/package.hpp"
#include "tetrodotoxin/model/terminal.hpp"

namespace Tetrodotoxin::Model::Packages {

// Compiled is the optional Package capability for completed terminal products.
// It exposes the useful result without leaking Archiver tables, repository
// identity, or filesystem policy onto the common Package contract.
class Compiled : public Model::Package {
 public:
  using ContractOwner = Compiled;
  static constexpr Perimortem::System::Uuid contract_id{
    0x28e510c6d26d45b4,
    0x8a26a7bc02bf0202,
  };

  constexpr auto implements(Perimortem::System::Uuid requested) const
      -> Bool override {
    return requested == contract_id || Package::implements(requested);
  }

  virtual constexpr auto get_terminals() const
      -> Perimortem::Core::View::Vector<Model::Terminal> = 0;
};

}  // namespace Tetrodotoxin::Model::Packages
