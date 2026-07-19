// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "ttx/model/types/terminal.hpp"

namespace Ttx::Model::Types {

// Signed is the Terminal contract for a two's-complement integer domain.
// Concrete width Types supply their fixed name, representation, and
// documentation.
class Signed : public Terminal {
 public:
  using ContractOwner = Signed;
  static constexpr Perimortem::System::Uuid contract_id{
    0xd6027af70ac64c12,
    0x8a76a2ead91b2550,
  };

  constexpr auto implements(Perimortem::System::Uuid requested) const
      -> Bool override {
    return requested == contract_id || Terminal::implements(requested);
  }
};

}  // namespace Ttx::Model::Types
