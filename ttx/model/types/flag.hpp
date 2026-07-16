// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "ttx/model/types/terminal.hpp"

namespace Ttx::Model::Types {

// Flag is the Terminal contract for a binary logical domain. Its storage
// size is toolchain-defined and does not dictate the chosen instruction width.
// On most reasonable architectures it should be assumed the size is 1 byte, but
// leaving it up to the toolchain allows for optimizations such as bit packing
// flags into bit arrays.
class Flag : public Terminal {
 public:
  using ContractOwner = Flag;
  static constexpr Perimortem::System::Uuid contract_id{
    0xe473697ddb94453b,
    0x8a4d310f695ed0cc,
  };

  auto implements(Perimortem::System::Uuid requested) const -> Bool override {
    return requested == contract_id || Terminal::implements(requested);
  }
};

}  // namespace Ttx::Model::Types
