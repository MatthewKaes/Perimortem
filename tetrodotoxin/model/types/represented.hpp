// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "ttx/model/type.hpp"

namespace Tetrodotoxin::Model::Types {

// Represented proves an explicit Library-Type to Shader-Type semantic edge.
// Structural coincidence never establishes this contract. The target planner
// still owns physical Representation and may reject either Type.
class Represented : public Ttx::Model::Type {
 public:
  using ContractOwner = Represented;
  static constexpr Perimortem::System::Uuid contract_id{
    0xcf2d18ca329c42be,
    0x94250f3c60ff428b,
  };

  constexpr auto implements(Perimortem::System::Uuid requested) const
      -> Bool override {
    return requested == contract_id || Ttx::Model::Type::implements(requested);
  }

  virtual constexpr auto get_shader_type() const -> const Ttx::Model::Type& = 0;
};

}  // namespace Tetrodotoxin::Model::Types
