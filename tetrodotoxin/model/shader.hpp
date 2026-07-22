// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "tetrodotoxin/model/render.hpp"
#include "ttx/concept/abstract.hpp"

namespace Tetrodotoxin::Model {

// Shader is the semantic contract for one validated implementation of a
// Render contract. Target products derive from its real Stage edges.
class Shader : public Ttx::Concept::Abstract {
 public:
  using ContractOwner = Shader;
  static constexpr Perimortem::System::Uuid contract_id{
    0x48fcf12f5df34165,
    0x8292fc72a80cb825,
  };
  constexpr auto implements(Perimortem::System::Uuid requested) const
      -> Bool override {
    return requested == contract_id || Abstract::implements(requested);
  }
  virtual constexpr auto get_render() const -> const Render& = 0;
  virtual constexpr auto get_stage_count() const -> Count = 0;
  virtual auto get_stage(Count index) const
      -> const Ttx::Concept::Abstract& = 0;
};

}  // namespace Tetrodotoxin::Model
