// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/option.hpp"

#include "perimortem/memory/allocator/arena.hpp"

#include "tetrodotoxin/scene/language/monograph.hpp"
#include "tetrodotoxin/terminal/graphics/products.hpp"
#include "ttx/model/type.hpp"

namespace Tetrodotoxin::Terminal::Graphics {

// Compiler derives one graphics runtime product after the Scene and its Host
// requirement have both completed. Configured Types name the runtime behavior
// available for this target, which keeps target selection outside the Scene
// graph while preserving exact Type identity at the handoff.
class Compiler {
 public:
  auto compile(
      Perimortem::Memory::Allocator::Arena& arena,
      const Tetrodotoxin::Scene::Language::Monograph& scene,
      const Ttx::Model::Type& requirement,
      Perimortem::Core::View::Vector<
          Ttx::Concept::Reference<const Ttx::Model::Type>> configured) const
      -> Perimortem::Core::Option<Products>;
};

}  // namespace Tetrodotoxin::Terminal::Graphics
