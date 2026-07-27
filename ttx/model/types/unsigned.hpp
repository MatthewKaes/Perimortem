// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "ttx/model/types/value.hpp"

namespace Ttx::Model::Types {

// Signed is the Terminal contract for a value that can store both positive and
// negative integer values.
class Unsigned : public Value {
 public:
  using ClassCatagory = Unsigned;
  static constexpr Perimortem::System::Uuid contract_id{
    0x0a8a00d5ed054be1,
    0x9e5d43885c4050b2,
  };

  constexpr auto implements(Perimortem::System::Uuid requested) const
      -> Bool override {
    return requested == contract_id || Value::implements(requested);
  }
};

}  // namespace Ttx::Model::Types
