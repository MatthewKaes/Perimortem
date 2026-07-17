// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/vector.hpp"

#include "ttx/concept/reference.hpp"
#include "ttx/model/layouts/fluid.hpp"
#include "ttx/model/pack.hpp"

namespace Ttx::Model::Packs {

// Positional is a Pack that owns its Fluid fitting view. The source owner
// flattens nested Positional Packs before construction. Every other Abstract
// remains one value, including typed aggregate Expressions and Named Packs.
//
// Positional borrows an already normalized entry view. It neither allocates a
// copied list nor acquires Layout as a public semantic contract.
class Positional final : public Pack {
 public:
  using ContractOwner = Positional;
  static constexpr Perimortem::System::Uuid contract_id{
    0x71831cf5a44e4e51,
    0x8ca2d9a8b84bc220,
  };

  Positional(
      Perimortem::Core::View::Vector<Concept::Reference<Concept::Abstract>>
          values);

  auto implements(Perimortem::System::Uuid requested) const -> Bool override;
  auto get_layout() const -> const Layouts::Fluid& override;

 private:
  Layouts::Fluid layout;
};

}  // namespace Ttx::Model::Packs
