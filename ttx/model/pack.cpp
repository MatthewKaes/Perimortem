// Perimortem Engine
// Copyright © Matt Kaes

#include "ttx/model/pack.hpp"

#include "ttx/concept/invalid.hpp"

auto Ttx::Model::Pack::get_name() const -> Perimortem::Core::View::Bytes {
  return {};
}

auto Ttx::Model::Pack::resolve_context(Perimortem::Core::View::Bytes) const
    -> const Concept::Abstract& {
  return Concept::Invalid::get_invalid();
}
