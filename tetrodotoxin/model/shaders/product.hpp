// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/memory/allocator/arena.hpp"
#include "perimortem/memory/managed/vector.hpp"

#include "tetrodotoxin/model/shader.hpp"
#include "ttx/concept/reference.hpp"

namespace Tetrodotoxin::Model::Shaders {

// Product is the Package-local relation between one real Shader graph and its
// completed terminal records. Terminal ordinals are stable only within the
// owning Compiled Package. Neither authored names nor logical paths select the
// Shader. Archiver persists these edges after it has restored both owners.
class Product {
 public:
  class Stage {
   public:
    constexpr Stage(
        const Ttx::Concept::Abstract& stage,
        Count module_terminal,
        Count interface_terminal)
        : stage(stage),
          module_terminal(module_terminal),
          interface_terminal(interface_terminal) {}

    constexpr auto get_stage() const -> const Ttx::Concept::Abstract& {
      return stage.get();
    }
    constexpr auto get_module_terminal() const -> Count {
      return module_terminal;
    }
    constexpr auto get_interface_terminal() const -> Count {
      return interface_terminal;
    }

   private:
    Ttx::Concept::Reference<Ttx::Concept::Abstract> stage;
    Count module_terminal;
    Count interface_terminal;
  };

  Product(
      Perimortem::Memory::Allocator::Arena& arena,
      const Model::Shader& shader,
      Perimortem::Core::View::Vector<Stage> stages);

  constexpr auto get_shader() const -> const Model::Shader& {
    return shader.get();
  }
  constexpr auto get_stages() const -> Perimortem::Core::View::Vector<Stage> {
    return stages;
  }

 private:
  Ttx::Concept::Reference<Model::Shader> shader;
  Perimortem::Memory::Managed::Vector<Stage> stages;
};

}  // namespace Tetrodotoxin::Model::Shaders
