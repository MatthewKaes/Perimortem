// Perimortem Engine
// Copyright © Matt Kaes

#include "ttx/model/pack.hpp"

#include "ttx/abstraction/invalid.hpp"

auto Ttx::Model::Pack::get_name() const -> Perimortem::Core::View::Bytes {
  return {};
}

auto Ttx::Model::Pack::resolve_context(Perimortem::Core::View::Bytes) const
    -> const Abstraction::Abstract& {
  static const Abstraction::Invalid invalid;
  return invalid;
}
