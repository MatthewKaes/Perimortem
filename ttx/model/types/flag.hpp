// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "ttx/model/types/terminal.hpp"

namespace Ttx::Model::Types {

// Flag is the Terminal contract for a binary logical domain. Concrete logical
// Types supply their fixed name, representation, and documentation.
class Flag : public Terminal {
 public:
  using ContractOwner = Flag;
  static constexpr Perimortem::System::Uuid contract_id{
    0xe473697ddb94453b,
    0x8a4d310f695ed0cc,
  };

  constexpr auto implements(Perimortem::System::Uuid requested) const
      -> Bool override {
    return requested == contract_id || Terminal::implements(requested);
  }
};

}  // namespace Ttx::Model::Types
