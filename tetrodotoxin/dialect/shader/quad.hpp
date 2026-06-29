// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/static/vector.hpp"

#include "tetrodotoxin/syntax/type/declaration.hpp"

namespace Tetrodotoxin::Dialect::Shader {

class Vec2Literal {
 public:
  Real_32 x = 0.0f;
  Real_32 y = 0.0f;
};

class QuadMesh {
 public:
  static constexpr Count vertex_count = 6;

  static auto default_mesh() -> QuadMesh;
  static auto from_type(const Syntax::Type::Declaration& type) -> QuadMesh;

  Perimortem::Core::Static::Vector<Vec2Literal, vertex_count> positions;
  Perimortem::Core::Static::Vector<Vec2Literal, vertex_count> uvs;
};

}  // namespace Tetrodotoxin::Dialect::Shader
