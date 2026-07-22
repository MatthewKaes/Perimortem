// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/model/stages/required.hpp"

#include "ttx/concept/invalid.hpp"

using namespace Perimortem::Core;
using namespace Ttx::Concept;

Tetrodotoxin::Model::Stages::Required::Required(
    Perimortem::Memory::Allocator::Arena& arena,
    View::Bytes name,
    Execution execution,
    View::Vector<Reference<Ttx::Model::Addressable>> parameters,
    View::Vector<Reference<Ttx::Model::Addressable>> results,
    View::Vector<Reference<Ttx::Model::Addressable>> reads,
    const Documentation& documentation)
    : name(arena.proxy(name)),
      execution(execution),
      documentation(documentation),
      parameters(arena),
      results(arena),
      reads(arena),
      parameter_values(arena),
      result_values(arena) {
  this->parameters.reset(parameters.get_size());
  parameter_values.reset(parameters.get_size());
  for (Count i = 0; i < parameters.get_size(); i++) {
    this->parameters.insert(parameters[i]);
    parameter_values.insert(Reference<Abstract>(parameters[i].get()));
  }
  this->results.reset(results.get_size());
  result_values.reset(results.get_size());
  for (Count i = 0; i < results.get_size(); i++) {
    this->results.insert(results[i]);
    result_values.insert(Reference<Abstract>(results[i].get()));
  }
  this->reads.reset(reads.get_size());
  for (Count i = 0; i < reads.get_size(); i++) {
    this->reads.insert(reads[i]);
  }
  parameter_layout = Ttx::Model::Layouts::Named(parameter_values.get_view());
  result_layout = Ttx::Model::Layouts::Named(result_values.get_view());
}

auto Tetrodotoxin::Model::Stages::Required::resolve_context(View::Bytes) const
    -> const Abstract& {
  return Invalid::get_invalid();
}

auto Tetrodotoxin::Model::Stages::Required::get_read(Count index) const
    -> const Abstract& {
  if (index >= reads.get_size()) {
    return Invalid::get_invalid();
  }
  return reads.get_view()[index].get();
}
