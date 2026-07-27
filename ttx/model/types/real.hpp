// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "ttx/model/types/value.hpp"

namespace Ttx::Model::Types {

// Real is the Terminal contract for an IEEE floating-point domain. Concrete
// precision Types supply their fixed name, representation, and documentation.
class Real : public Value {
 public:
  using ClassCatagory = Real;
  static constexpr Perimortem::System::Uuid contract_id{
    0xf83c87fdb02d4fe3,
    0x929dae6e905f5dfa,
  };

  constexpr auto implements(Perimortem::System::Uuid requested) const
      -> Bool override {
    return requested == contract_id || Value::implements(requested);
  }
};

}  // namespace Ttx::Model::Types
