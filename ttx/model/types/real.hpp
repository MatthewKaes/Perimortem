// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "ttx/model/types/terminal.hpp"

namespace Ttx::Model::Types {

// Real is the Terminal contract for a floating-point domain. The constructing
// toolchain determines its supported formats, sizes, alignments, and names.
class Real : public Terminal {
 public:
  using ContractOwner = Real;
  static constexpr Perimortem::System::Uuid contract_id{
    0xf83c87fdb02d4fe3,
    0x929dae6e905f5dfa,
  };

  auto implements(Perimortem::System::Uuid requested) const -> Bool override {
    return requested == contract_id || Terminal::implements(requested);
  }
};

}  // namespace Ttx::Model::Types
