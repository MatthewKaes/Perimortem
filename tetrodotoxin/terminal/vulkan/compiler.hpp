// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/option.hpp"

#include "perimortem/memory/allocator/arena.hpp"

#include "tetrodotoxin/shader/language/program.hpp"
#include "tetrodotoxin/terminal/vulkan/products.hpp"

namespace Tetrodotoxin::Terminal::Vulkan {

// Compiler reflects the Vulkan facts already proven by Render and Shader. The
// selected SPIR V symbol remains an external product locator, while pipeline
// layout and vertex input facts are derived from the completed graph.
class Compiler {
 public:
  auto compile(
      Perimortem::Memory::Allocator::Arena& arena,
      const Tetrodotoxin::Shader::Language::Program& program,
      Perimortem::Core::View::Bytes symbol) const
      -> Perimortem::Core::Option<Products>;
};

}  // namespace Tetrodotoxin::Terminal::Vulkan
