// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "ttx/model/types/terminal.hpp"

namespace Ttx::Model::Types {

// Unsigned is the Terminal contract for a non-negative integer domain. Width
// support and source names belong to the constructing toolchain.
class Unsigned : public Terminal {
 public:
  using ContractOwner = Unsigned;
  static constexpr Perimortem::System::Uuid contract_id{
    0x0a8a00d5ed054be1,
    0x9e5d43885c4050b2,
  };

  auto implements(Perimortem::System::Uuid requested) const -> Bool override {
    return requested == contract_id || Terminal::implements(requested);
  }
};

}  // namespace Ttx::Model::Types
