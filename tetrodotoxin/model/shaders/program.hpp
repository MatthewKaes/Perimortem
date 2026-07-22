// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/memory/allocator/arena.hpp"
#include "perimortem/memory/managed/map.hpp"
#include "perimortem/memory/managed/vector.hpp"

#include "tetrodotoxin/model/shader.hpp"
#include "tetrodotoxin/model/stages/implemented.hpp"
#include "ttx/concept/reference.hpp"

namespace Tetrodotoxin::Model::Shaders {

// Program is one validated implementation of a real Render contract. It owns
// the exact Stage identities whose common Bodies may produce target modules.
class Program final : public Model::Shader {
 public:
  static auto construct(
      Perimortem::Memory::Allocator::Arena& arena,
      Perimortem::Core::View::Bytes name,
      const Model::Render& render,
      Perimortem::Core::View::Vector<
          Ttx::Concept::Reference<Model::Stages::Implemented>> stages,
      const Ttx::Concept::Documentation& documentation)
      -> const Ttx::Concept::Abstract&;

  constexpr auto get_name() const -> Perimortem::Core::View::Bytes override {
    return name;
  }
  constexpr auto get_documentation() const
      -> const Ttx::Concept::Documentation& override {
    return documentation;
  }
  auto resolve_context(Perimortem::Core::View::Bytes route) const
      -> const Ttx::Concept::Abstract& override;
  constexpr auto get_render() const -> const Model::Render& override {
    return render.get();
  }
  constexpr auto get_stage_count() const -> Count override {
    return stages.get_size();
  }
  auto get_stage(Count index) const -> const Ttx::Concept::Abstract& override;

 private:
  Program(
      Perimortem::Memory::Allocator::Arena& arena,
      Perimortem::Core::View::Bytes name,
      const Model::Render& render,
      Perimortem::Core::View::Vector<
          Ttx::Concept::Reference<Model::Stages::Implemented>> stages,
      const Ttx::Concept::Documentation& documentation);

  Perimortem::Core::View::Bytes name;
  Ttx::Concept::Reference<Model::Render> render;
  const Ttx::Concept::Documentation& documentation;
  Perimortem::Memory::Managed::Vector<
      Ttx::Concept::Reference<Model::Stages::Implemented>>
      stages;
  Perimortem::Memory::Managed::Map<
      Perimortem::Core::View::Bytes,
      Ttx::Concept::Reference<Model::Stages::Implemented>>
      stages_by_name;
  Bool valid = True;
};

}  // namespace Tetrodotoxin::Model::Shaders
