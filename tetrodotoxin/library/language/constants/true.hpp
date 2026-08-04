// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "tetrodotoxin/library/language/constants/flag.hpp"

namespace Tetrodotoxin::Library::Language::Constants {

// True is the positive Flag Constant refinement. The retained Flag Type still
// owns representation while this identity exposes the closed logical value.
class True : public Flag {
 public:
  using ClassCatagory = True;
  static constexpr Perimortem::System::Uuid contract_id{
    0x622f43659a9a4cc5,
    0x935e114a9c2fc915,
  };

  constexpr True(const Ttx::Model::Types::Flag& type) : Flag(type, ::True) {}

  constexpr auto implements(Perimortem::System::Uuid requested) const
      -> Bool override {
    return requested == contract_id || Flag::implements(requested);
  }
};

}  // namespace Tetrodotoxin::Library::Language::Constants
