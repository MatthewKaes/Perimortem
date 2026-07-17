// Perimortem Engine
// Copyright © Matt Kaes

#include "ttx/model/packs/named.hpp"

Ttx::Model::Packs::Named::Named(
    Perimortem::Core::View::Vector<Concept::Reference<Concept::Abstract>>
        bindings)
    : layout(bindings) {}

auto Ttx::Model::Packs::Named::implements(
    Perimortem::System::Uuid requested) const -> Bool {
  return requested == contract_id || Pack::implements(requested);
}

auto Ttx::Model::Packs::Named::get_layout() const -> const Layouts::Named& {
  return layout;
}
