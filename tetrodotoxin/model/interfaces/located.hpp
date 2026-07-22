// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "tetrodotoxin/model/addressables/parameter.hpp"

namespace Tetrodotoxin::Model::Interfaces {

// Located is a Stage interface slot with an authored numeric location. The
// target planner consumes this fact. It is not inferred from declaration order
// or a formatted name.
class Located final : public Addressables::Parameter {
 public:
  using ContractOwner = Located;
  static constexpr Perimortem::System::Uuid contract_id{
    0xeab0d4c05bcc4b31,
    0x88ae8d3fdbe094dd,
  };

  constexpr Located(
      Perimortem::Core::View::Bytes name,
      const Ttx::Model::Type& type,
      Unsigned_32 location)
      : Parameter(name, type), location(location) {}

  constexpr auto implements(Perimortem::System::Uuid requested) const
      -> Bool override {
    return requested == contract_id || Parameter::implements(requested);
  }
  constexpr auto get_location() const -> Unsigned_32 { return location; }

 private:
  Unsigned_32 location;
};

}  // namespace Tetrodotoxin::Model::Interfaces
