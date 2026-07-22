// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/model/callables/self.hpp"

#include "ttx/concept/invalid.hpp"

using namespace Perimortem::Core;
using namespace Ttx::Concept;

Tetrodotoxin::Model::Callables::Self::Self(
    Perimortem::Memory::Allocator::Arena& arena,
    View::Bytes name,
    View::Vector<Reference<Ttx::Model::Addressable>> parameters,
    View::Vector<Reference<Abstract>> results,
    const Documentation& documentation)
    : name(arena.proxy(name)),
      documentation(documentation),
      parameters(arena),
      results(arena) {
  this->parameters.reset(parameters.get_size());
  for (Count i = 0; i < parameters.get_size(); i++) {
    this->parameters.insert(parameters[i]);
  }
  this->results.reset(results.get_size());
  for (Count i = 0; i < results.get_size(); i++) {
    this->results.insert(results[i]);
  }
  parameter_layout = Ttx::Model::Layouts::Structured(this->parameters);
  result_layout = Ttx::Model::Layouts::Fluid(this->results);
}

auto Tetrodotoxin::Model::Callables::Self::resolve_context(View::Bytes) const
    -> const Abstract& {
  return Invalid::get_invalid();
}
