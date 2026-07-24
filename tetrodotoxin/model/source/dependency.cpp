// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/model/source/dependency.hpp"

#include "ttx/concept/invalid.hpp"

auto Tetrodotoxin::Model::Dependency::resolve_context(
    Perimortem::Core::View::Bytes) const -> const Ttx::Concept::Abstract& {
  // Dependency is the retained locator edge rather than the visible binding.
  // Source publishes get_binding(), which owns the local name and forwards
  // lookup only into the dependency's closed Exports surface.
  return Ttx::Concept::Invalid::get_invalid();
}
