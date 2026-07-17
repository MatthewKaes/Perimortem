// Perimortem Engine
// Copyright © Matt Kaes

#include "ttx/model/packs/positional.hpp"

Ttx::Model::Packs::Positional::Positional(
    Perimortem::Core::View::Vector<Concept::Reference<Concept::Abstract>>
        values)
    : layout(values) {}

auto Ttx::Model::Packs::Positional::implements(
    Perimortem::System::Uuid requested) const -> Bool {
  return requested == contract_id || Pack::implements(requested);
}

auto Ttx::Model::Packs::Positional::get_layout() const
    -> const Layouts::Fluid& {
  return layout;
}
