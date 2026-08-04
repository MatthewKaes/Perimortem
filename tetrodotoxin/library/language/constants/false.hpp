// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "tetrodotoxin/library/language/constants/flag.hpp"

namespace Tetrodotoxin::Library::Language::Constants {

// False is the negative Flag Constant refinement. It shares Flag equality and
// fitting while category proof can still select the exact logical value.
class False : public Flag {
 public:
  using ClassCatagory = False;
  static constexpr Perimortem::System::Uuid contract_id{
    0x07c1acb57a07401a,
    0x803ffb29898ccb31,
  };

  constexpr False(const Ttx::Model::Types::Flag& type) : Flag(type, ::False) {}

  constexpr auto implements(Perimortem::System::Uuid requested) const
      -> Bool override {
    return requested == contract_id || Flag::implements(requested);
  }
};

}  // namespace Tetrodotoxin::Library::Language::Constants
