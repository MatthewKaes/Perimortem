// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/model/packages/precompiled.hpp"

using namespace Perimortem::Core;
using namespace Ttx::Concept;

Tetrodotoxin::Model::Packages::Precompiled::Precompiled(
    Perimortem::Memory::Allocator::Arena& arena,
    const Model::Namespace& exports,
    View::Vector<Reference<Model::Package>> dependencies)
    : exports(exports), dependencies(arena) {
  for (Count i = 0; i < dependencies.get_size(); i++) {
    this->dependencies.insert(dependencies[i]);
  }
}

auto Tetrodotoxin::Model::Packages::Precompiled::resolve_context(
    View::Bytes route) const -> const Abstract& {
  return exports.resolve_context(route);
}
