// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/memory/allocator/arena.hpp"
#include "perimortem/memory/managed/vector.hpp"

#include "tetrodotoxin/model/namespace.hpp"
#include "tetrodotoxin/model/render.hpp"
#include "tetrodotoxin/model/renderables/value.hpp"
#include "tetrodotoxin/model/stages/required.hpp"
#include "tetrodotoxin/model/types/members.hpp"
#include "ttx/concept/reference.hpp"
#include "ttx/model/layouts/structured.hpp"

namespace Tetrodotoxin::Model::Renders {

// Contract is one complete Render Type. It retains value state, role
// namespaces, and the exact Stage signatures a Shader must implement.
class Contract final : public Model::Render {
 public:
  static auto construct(
      Perimortem::Memory::Allocator::Arena& arena,
      Perimortem::Core::View::Bytes name,
      Perimortem::Core::View::Vector<
          Ttx::Concept::Reference<Model::Renderables::Value>> values,
      const Model::Namespace& constants,
      const Model::Namespace& pushes,
      const Model::Namespace& resources,
      Perimortem::Core::View::Vector<
          Ttx::Concept::Reference<Model::Stages::Required>> stages,
      const Ttx::Concept::Documentation& documentation)
      -> const Ttx::Concept::Abstract&;

  constexpr auto get_name() const -> Perimortem::Core::View::Bytes override {
    return name;
  }
  constexpr auto get_documentation() const
      -> const Ttx::Concept::Documentation& override {
    return documentation;
  }
  constexpr auto get_layout() const -> const Ttx::Concept::Layout& override {
    return layout;
  }
  auto resolve_context(Perimortem::Core::View::Bytes route) const
      -> const Ttx::Concept::Abstract& override;

  constexpr auto get_constant_count() const -> Count override {
    return constants.get_export_count();
  }
  auto get_constant(Count index) const
      -> const Ttx::Concept::Abstract& override;
  constexpr auto get_push_count() const -> Count override {
    return pushes.get_export_count();
  }
  auto get_push(Count index) const -> const Ttx::Concept::Abstract& override;
  constexpr auto get_resource_count() const -> Count override {
    return resources.get_export_count();
  }
  auto get_resource(Count index) const
      -> const Ttx::Concept::Abstract& override;
  constexpr auto get_stage_count() const -> Count override {
    return stages.get_size();
  }
  auto get_stage(Count index) const -> const Ttx::Concept::Abstract& override;

  // These Namespace identities are part of this concrete Render dialect fact.
  // The archive retains them directly so restored resolution does not invent
  // group names or rebuild presentation from flattened child lists.
  constexpr auto get_constants_namespace() const
      -> const Model::Namespace& override {
    return constants;
  }
  constexpr auto get_pushes_namespace() const
      -> const Model::Namespace& override {
    return pushes;
  }
  constexpr auto get_resources_namespace() const
      -> const Model::Namespace& override {
    return resources;
  }

 private:
  Contract(
      Perimortem::Memory::Allocator::Arena& arena,
      Perimortem::Core::View::Bytes name,
      Perimortem::Core::View::Vector<
          Ttx::Concept::Reference<Model::Renderables::Value>> values,
      const Model::Namespace& constants,
      const Model::Namespace& pushes,
      const Model::Namespace& resources,
      Perimortem::Core::View::Vector<
          Ttx::Concept::Reference<Model::Stages::Required>> stages,
      const Ttx::Concept::Documentation& documentation);

  Perimortem::Core::View::Bytes name;
  const Ttx::Concept::Documentation& documentation;
  Types::Members members;
  Perimortem::Memory::Managed::Vector<
      Ttx::Concept::Reference<Model::Renderables::Value>>
      values;
  const Model::Namespace& constants;
  const Model::Namespace& pushes;
  const Model::Namespace& resources;
  Perimortem::Memory::Managed::Vector<
      Ttx::Concept::Reference<Model::Stages::Required>>
      stages;
  Perimortem::Memory::Managed::Vector<
      Ttx::Concept::Reference<Ttx::Model::Addressable>>
      layout_values;
  Ttx::Model::Layouts::Structured layout;
  Bool valid = True;
};

}  // namespace Tetrodotoxin::Model::Renders
