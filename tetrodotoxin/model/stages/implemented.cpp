// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/model/stages/implemented.hpp"

#include "ttx/concept/invalid.hpp"

using namespace Perimortem::Core;
using namespace Ttx::Concept;

Tetrodotoxin::Model::Stages::Implemented::Implemented(
    Perimortem::Memory::Allocator::Arena& arena,
    const Required& required,
    View::Vector<Reference<Ttx::Model::Addressable>> parameters,
    View::Vector<Reference<Ttx::Model::Addressable>> results,
    Ttx::Model::Body body,
    const Documentation& documentation)
    : required(required),
      documentation(documentation),
      parameters(arena),
      results(arena),
      body(body) {
  this->parameters.reset(parameters.get_size());
  for (Count i = 0; i < parameters.get_size(); i++) {
    this->parameters.insert(Reference<Abstract>(parameters[i].get()));
  }
  this->results.reset(results.get_size());
  for (Count i = 0; i < results.get_size(); i++) {
    this->results.insert(Reference<Abstract>(results[i].get()));
  }
  parameter_layout = Ttx::Model::Layouts::Named(this->parameters.get_view());
  result_layout = Ttx::Model::Layouts::Named(this->results.get_view());
}

auto Tetrodotoxin::Model::Stages::Implemented::resolve_context(
    View::Bytes) const -> const Abstract& {
  return Invalid::get_invalid();
}
