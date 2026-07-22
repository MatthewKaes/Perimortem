// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "tetrodotoxin/model/render.hpp"
#include "tetrodotoxin/model/shader.hpp"
#include "ttx/concept/reference.hpp"

namespace Tetrodotoxin::Model::Apps {

// Binding is one explicit App selection edge. Shader discovery and spelling
// never select policy: the selected Shader must implement this exact Render
// identity.
class Binding {
 public:
  constexpr Binding(const Model::Render& render, const Model::Shader& shader)
      : render(render), shader(shader) {}

  constexpr auto get_render() const -> const Model::Render& {
    return render.get();
  }
  constexpr auto get_shader() const -> const Model::Shader& {
    return shader.get();
  }

 private:
  Ttx::Concept::Reference<Model::Render> render;
  Ttx::Concept::Reference<Model::Shader> shader;
};

}  // namespace Tetrodotoxin::Model::Apps
