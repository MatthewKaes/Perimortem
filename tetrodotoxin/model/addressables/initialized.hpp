// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "tetrodotoxin/model/addressables/field.hpp"
#include "ttx/concept/reference.hpp"
#include "ttx/model/expression.hpp"

namespace Tetrodotoxin::Model::Addressables {

// Initialized is the narrower Field contract for a complete authored default
// expression. Fields without a default remain ordinary Field owners, avoiding
// a nullable or Invalid sentinel for semantic absence.
class Initialized final : public Field {
 public:
  using ContractOwner = Initialized;
  static constexpr Perimortem::System::Uuid contract_id{
    0xe4dc9fe35c4b453e,
    0xb74dd3d64a38d039,
  };

  constexpr Initialized(
      Perimortem::Core::View::Bytes name,
      const Ttx::Model::Type& type,
      const Ttx::Model::Expression& initializer,
      const Ttx::Concept::Documentation& documentation =
          Ttx::Concept::Documentation::get_empty())
      : Field(name, type, documentation), initializer(initializer) {}

  constexpr auto implements(Perimortem::System::Uuid requested) const
      -> Bool override {
    return requested == contract_id || Field::implements(requested);
  }
  constexpr auto get_initializer() const -> const Ttx::Model::Expression& {
    return initializer.get();
  }

 private:
  Ttx::Concept::Reference<Ttx::Model::Expression> initializer;
};

}  // namespace Tetrodotoxin::Model::Addressables
