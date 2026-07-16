// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "ttx/model/types/terminal.hpp"

namespace Ttx::Model::Types {

// Signed is the Terminal contract for a signed integer domain. Width support
// and source names belong to the constructing toolchain.
class Signed : public Terminal {
 public:
  using ContractOwner = Signed;
  static constexpr Perimortem::System::Uuid contract_id{
    0xd6027af70ac64c12,
    0x8a76a2ead91b2550,
  };

  auto implements(Perimortem::System::Uuid requested) const -> Bool override {
    return requested == contract_id || Terminal::implements(requested);
  }
};

}  // namespace Ttx::Model::Types
