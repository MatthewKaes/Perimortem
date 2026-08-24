// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "perimortem/graphics/frame/batch.hpp"

using namespace Perimortem;

Graphics::Frame::Batch::Batch(
    Program program,
    Core::View::Vector<Core::Object<>> resources,
    Core::View::Bytes inputs,
    Transform transform,
    Count vertex_count,
    S64 z_index,
    Count authored_order)
    : program(program),
      inputs(inputs),
      transform(transform),
      vertex_count(vertex_count),
      z_index(z_index),
      authored_order(authored_order) {
  for (Core::Object<> resource : resources) {
    this->resources.emplace(Resource(resource));
  }
}

auto Graphics::Frame::Batch::get_inputs() const -> Core::View::Bytes {
  return inputs.get_view();
}
