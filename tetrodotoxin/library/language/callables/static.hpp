// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "ttx/model/callable.hpp"

namespace Tetrodotoxin::Library::Language::Callables {

// Static is the invocation distinction for a Callable with no runtime
// receiver. The owning declaration decides how that Callable is reached.
class Static : public Ttx::Model::Callable {
 public:
  using ClassCatagory = Static;
  static constexpr Perimortem::System::Uuid contract_id{
    0x5911783005f1489b,
    0x8dd2193e3cdaf357,
  };

  constexpr auto implements(Perimortem::System::Uuid requested) const
      -> Bool override {
    return requested == contract_id || Callable::implements(requested);
  }
};

}  // namespace Tetrodotoxin::Library::Language::Callables
