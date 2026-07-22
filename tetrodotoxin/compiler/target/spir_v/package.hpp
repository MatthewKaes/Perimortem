// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/memory/allocator/arena.hpp"
#include "perimortem/memory/managed/vector.hpp"

#include "tetrodotoxin/model/namespace.hpp"
#include "tetrodotoxin/model/shaders/product.hpp"
#include "tetrodotoxin/model/terminal.hpp"

namespace Tetrodotoxin::Target::SpirV {

// PackageCompiler finds real exported Shader identities and turns every Stage
// into terminal products. It follows Alias and Namespace edges only. It neither
// searches source nor invents a shader registry.
class PackageCompiler {
 public:
  static auto compile(
      Perimortem::Memory::Allocator::Arena& arena,
      const Model::Namespace& exports,
      Perimortem::Memory::Managed::Vector<Model::Terminal>& terminals,
      Perimortem::Memory::Managed::Vector<Model::Shaders::Product>& products)
      -> Bool;
};

}  // namespace Tetrodotoxin::Target::SpirV
