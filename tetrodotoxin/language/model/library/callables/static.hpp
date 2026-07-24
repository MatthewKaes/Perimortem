// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "ttx/model/callable.hpp"

namespace Ttx::Model::Callables {

// Static is the invocation distinction for a Callable selected through a Type
// or package context. Invocation does not add a receiver.
class Static : public Callable {
 public:
  using ContractOwner = Static;
  static constexpr Perimortem::System::Uuid contract_id{
    0x5911783005f1489b,
    0x8dd2193e3cdaf357,
  };

  constexpr auto implements(Perimortem::System::Uuid requested) const
      -> Bool override {
    return requested == contract_id || Callable::implements(requested);
  }
};

}  // namespace Ttx::Model::Callables
