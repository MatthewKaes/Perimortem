// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "ttx/model/callable.hpp"

namespace Ttx::Model {

// Self is the invocation distinction for a Callable selected through an
// addressable value. The receiver is entry zero of get_parameters(); no
// parser, compiler, reflector, or ABI layer may prepend it a second time.
class Self : public Callable {
 public:
  using ContractOwner = Self;
  static constexpr Perimortem::System::Uuid contract_id{
    0xc654e5492f59488a,
    0x94be43b98183e6ab,
  };

  auto implements(Perimortem::System::Uuid requested) const -> Bool override {
    return requested == contract_id || Callable::implements(requested);
  }
};

}  // namespace Ttx::Model
