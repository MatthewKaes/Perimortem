// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/model/expressions/call.hpp"

using namespace Perimortem::Core;

Tetrodotoxin::Model::Expressions::Call::Call(
    Perimortem::Memory::Allocator::Arena& arena,
    const Ttx::Model::Callable& callable,
    const Ttx::Model::Type& type,
    View::Vector<Ttx::Concept::Reference<Ttx::Model::Expression>> arguments)
    : callable(callable), type(type), arguments(arena) {
  this->arguments.reset(arguments.get_size());
  for (Count i = 0; i < arguments.get_size(); i++) {
    this->arguments.insert(
        Ttx::Concept::Reference<Ttx::Concept::Abstract>(arguments[i].get()));
  }
  inputs = Ttx::Model::Layouts::Fluid(this->arguments.get_view());
}
