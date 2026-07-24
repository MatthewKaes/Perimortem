// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/memory/allocator/arena.hpp"
#include "perimortem/memory/managed/map.hpp"
#include "perimortem/memory/managed/vector.hpp"

#include "tetrodotoxin/model/namespace.hpp"
#include "tetrodotoxin/model/package/compiled.hpp"

namespace Tetrodotoxin::Model::Package {

// Precompiled is the Package graph reconstructed from a Puffer Buffer. The
// Archiver owns how records become real Abstract objects. This class exposes
// the restored public graph without pretending the original Sources survived.
class Precompiled : public Compiled {
 public:
  Precompiled(
      Perimortem::Memory::Allocator::Arena& arena,
      const Model::Namespace& exports,
      Perimortem::Core::View::Vector<Ttx::Concept::Reference<Resolved>>
          dependencies = {},
      Perimortem::Core::View::Vector<
          Ttx::Concept::Reference<Ttx::Concept::Abstract>> definitions = {},
      Perimortem::Core::View::Vector<Model::Terminal> terminals = {},
      Perimortem::Core::View::Vector<Model::Shaders::Product> shader_products =
          {});

  auto get_documentation() const
      -> const Ttx::Concept::Documentation& override {
    return exports.get_documentation();
  }

  auto get_export_count() const -> Count override {
    return exports.get_export_count();
  }

  auto get_export(Count index) const -> const Ttx::Concept::Abstract& override {
    return exports.get_export(index);
  }

  auto get_dependencies() const -> Perimortem::Core::View::Vector<
      Ttx::Concept::Reference<Resolved>> override {
    return dependencies;
  }

  auto get_terminals() const
      -> Perimortem::Core::View::Vector<Model::Terminal> override {
    return terminals;
  }

  auto get_shader_products() const
      -> Perimortem::Core::View::Vector<Model::Shaders::Product> override {
    return shader_products;
  }

  auto get_definition_count() const -> Count override {
    return definitions.get_size();
  }

  auto get_definition(Count id) const -> const Ttx::Concept::Abstract& override;

  auto get_definition_id(const Ttx::Concept::Abstract& definition) const
      -> Count override;

  auto resolve_context(Perimortem::Core::View::Bytes route) const
      -> const Ttx::Concept::Abstract& override;

 private:
  const Model::Namespace& exports;
  Perimortem::Memory::Managed::Vector<Ttx::Concept::Reference<Resolved>>
      dependencies;
  Perimortem::Memory::Managed::Vector<
      Ttx::Concept::Reference<Ttx::Concept::Abstract>>
      definitions;
  Perimortem::Memory::Managed::Map<const Ttx::Concept::Abstract*, Count>
      definition_index;
  Perimortem::Memory::Managed::Vector<Model::Terminal> terminals;
  Perimortem::Memory::Managed::Vector<Model::Shaders::Product> shader_products;
};

}  // namespace Tetrodotoxin::Model::Package
