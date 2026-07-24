// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "tetrodotoxin/model/callables/self.hpp"

namespace Tetrodotoxin::Model::Callables {

// Sample is the Library-owned intrinsic contract for sampling an opaque Render
// resource. Shader lowering selects it by contract identity, never by the
// authored function or Type name.
class Sample : public Self {
 public:
  using Self::Self;
  using ContractOwner = Sample;
  static constexpr Perimortem::System::Uuid contract_id{
    0xa716a18bb1dc48d2,
    0xb5cbfe9105460297,
  };

  constexpr auto implements(Perimortem::System::Uuid requested) const
      -> Bool override {
    return requested == contract_id || Self::implements(requested);
  }
};

}  // namespace Tetrodotoxin::Model::Callables
