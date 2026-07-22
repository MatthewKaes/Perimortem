// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/model/shaders/product.hpp"

using namespace Perimortem::Core;

Tetrodotoxin::Model::Shaders::Product::Product(
    Perimortem::Memory::Allocator::Arena& arena,
    const Model::Shader& shader,
    View::Vector<Stage> stages)
    : shader(shader), stages(arena) {
  this->stages.reset(stages.get_size());
  for (Count i = 0; i < stages.get_size(); i++) {
    this->stages.insert(stages[i]);
  }
}
