// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/model/packages/precompiled.hpp"

#include "ttx/concept/invalid.hpp"

using namespace Perimortem::Core;
using namespace Ttx::Concept;

Tetrodotoxin::Model::Packages::Precompiled::Precompiled(
    Perimortem::Memory::Allocator::Arena& arena,
    const Model::Namespace& exports,
    View::Vector<Reference<Model::Package>> dependencies,
    View::Vector<Reference<Abstract>> definitions,
    View::Vector<Model::Terminal> terminals)
    : exports(exports),
      dependencies(arena),
      definitions(arena),
      definition_index(arena),
      terminals(arena) {
  for (Count i = 0; i < dependencies.get_size(); i++) {
    this->dependencies.insert(dependencies[i]);
  }

  this->definitions.reset(definitions.get_size());
  for (Count i = 0; i < definitions.get_size(); i++) {
    this->definitions.insert(definitions[i]);
    definition_index.insert(&definitions[i].get(), i);
  }

  for (Count i = 0; i < terminals.get_size(); i++) {
    this->terminals.insert(
        Model::Terminal(
            arena, terminals[i].get_path(), terminals[i].get_content()));
  }
}

auto Tetrodotoxin::Model::Packages::Precompiled::get_definition(Count id) const
    -> const Abstract& {
  if (id >= definitions.get_size()) {
    return Invalid::get_invalid();
  }

  return definitions.get_view()[id].get();
}

auto Tetrodotoxin::Model::Packages::Precompiled::get_definition_id(
    const Abstract& definition) const -> Count {
  const auto* selected = definition_index.find(&definition);
  return selected == nullptr ? Count(-1) : selected->value;
}

auto Tetrodotoxin::Model::Packages::Precompiled::resolve_context(
    View::Bytes route) const -> const Abstract& {
  return exports.resolve_context(route);
}
