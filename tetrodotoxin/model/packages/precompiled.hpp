// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/memory/allocator/arena.hpp"
#include "perimortem/memory/managed/map.hpp"
#include "perimortem/memory/managed/vector.hpp"

#include "tetrodotoxin/model/namespace.hpp"
#include "tetrodotoxin/model/packages/compiled.hpp"

namespace Tetrodotoxin::Model::Packages {

// Precompiled is the Package graph reconstructed from a Puffer Buffer. The
// Archiver owns how records become real Abstract objects. This class exposes
// the restored public graph without pretending the original Sources survived.
class Precompiled final : public Compiled {
 public:
  Precompiled(
      Perimortem::Memory::Allocator::Arena& arena,
      const Model::Namespace& exports,
      Perimortem::Core::View::Vector<Ttx::Concept::Reference<Model::Package>>
          dependencies = {},
      Perimortem::Core::View::Vector<
          Ttx::Concept::Reference<Ttx::Concept::Abstract>> definitions = {},
      Perimortem::Core::View::Vector<Model::Terminal> terminals = {});

  constexpr auto get_documentation() const
      -> const Ttx::Concept::Documentation& override {
    return exports.get_documentation();
  }

  constexpr auto get_export_count() const -> Count override {
    return exports.get_export_count();
  }

  constexpr auto get_export(Count index) const
      -> const Ttx::Concept::Abstract& override {
    return exports.get_export(index);
  }

  constexpr auto get_dependencies() const -> Perimortem::Core::View::Vector<
      Ttx::Concept::Reference<Model::Package>> override {
    return dependencies;
  }

  constexpr auto get_terminals() const
      -> Perimortem::Core::View::Vector<Model::Terminal> override {
    return terminals;
  }

  constexpr auto get_definition_count() const -> Count override {
    return definitions.get_size();
  }

  auto get_definition(Count id) const -> const Ttx::Concept::Abstract& override;

  auto get_definition_id(const Ttx::Concept::Abstract& definition) const
      -> Count override;

  auto resolve_context(Perimortem::Core::View::Bytes route) const
      -> const Ttx::Concept::Abstract& override;

 private:
  const Model::Namespace& exports;
  Perimortem::Memory::Managed::Vector<Ttx::Concept::Reference<Model::Package>>
      dependencies;
  Perimortem::Memory::Managed::Vector<
      Ttx::Concept::Reference<Ttx::Concept::Abstract>>
      definitions;
  Perimortem::Memory::Managed::Map<const Ttx::Concept::Abstract*, Count>
      definition_index;
  Perimortem::Memory::Managed::Vector<Model::Terminal> terminals;
};

}  // namespace Tetrodotoxin::Model::Packages
