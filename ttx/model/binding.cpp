// Perimortem Engine
// Copyright © Matt Kaes

#include "ttx/model/binding.hpp"

auto Ttx::Model::Binding::get_documentation() const
    -> const Concept::Documentation& {
  return Concept::Documentation::get_empty();
}
