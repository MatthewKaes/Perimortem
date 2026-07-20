// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/model/packages/sources.hpp"

#include "tetrodotoxin/model/dependencies/package.hpp"
#include "tetrodotoxin/model/dependencies/source.hpp"

using namespace Perimortem::Core;
using namespace Ttx::Concept;

Tetrodotoxin::Model::Packages::Sources::Sources(
    Perimortem::Memory::Allocator::Arena& arena,
    const Model::Source& source,
    const Model::Namespace& exports)
    : exports(exports), sources(arena), dependencies(arena) {
  collect_source(source);
}

auto Tetrodotoxin::Model::Packages::Sources::resolve_context(
    View::Bytes route) const -> const Abstract& {
  return exports.resolve_context(route);
}

auto Tetrodotoxin::Model::Packages::Sources::collect_source(
    const Model::Source& source) -> void {
  // Insert before walking edges so repeated imports and source cycles stop at
  // the first visit. The resulting order follows the first authored path from
  // the Package root.
  for (Count i = 0; i < sources.get_size(); i++) {
    if (&sources[i].get() == &source) {
      return;
    }
  }

  sources.insert(Reference<Model::Source>(source));
  const View::Vector<Reference<Model::Dependency>> edges =
      source.get_dependencies();
  for (Count i = 0; i < edges.get_size(); i++) {
    const Model::Dependency& edge = edges[i].get();

    // Source dependencies join the interpreted closure because their source
    // graph remains available. Package dependencies stay as package edges and
    // never expose whether their own Sources survived compilation.
    if (edge.is<Model::Dependencies::Source>()) {
      collect_source(edge.assume<Model::Dependencies::Source>().get_source());
      continue;
    }
    if (!edge.is<Model::Dependencies::Package>()) {
      continue;
    }

    const Model::Package& package =
        edge.assume<Model::Dependencies::Package>().get_package();
    Bool present = False;
    for (Count k = 0; k < dependencies.get_size(); k++) {
      if (&dependencies[k].get() == &package) {
        present = True;
        break;
      }
    }
    if (!present) {
      dependencies.insert(Reference<Model::Package>(package));
    }
  }
}
