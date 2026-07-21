// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/memory/allocator/arena.hpp"
#include "perimortem/memory/managed/map.hpp"
#include "perimortem/memory/managed/vector.hpp"

#include "tetrodotoxin/model/namespace.hpp"
#include "tetrodotoxin/model/packages/interpreted.hpp"

namespace Tetrodotoxin::Model::Packages {

// Sources is Tetrodotoxin's concrete Package assembled from an explicit list
// selected by a package container. Source owns no import graph, so membership
// cannot be inferred or accidentally multiplied. Every member must borrow the
// same completed Environment. The Package copies that Environment's distinct
// Package dependencies and builds a canonical definition table for archive
// linkage without performing repository or locator searches.
//
// Interpreted remains the capability, so hosts can provide alternative
// source backed Package implementations without changing Package consumers.
class Sources final : public Interpreted {
 public:
  static auto construct(
      Perimortem::Memory::Allocator::Arena& arena,
      Perimortem::Core::View::Vector<Ttx::Concept::Reference<Model::Source>>
          sources,
      const Model::Namespace& exports) -> const Ttx::Concept::Abstract&;

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

  constexpr auto get_sources() const -> Perimortem::Core::View::Vector<
      Ttx::Concept::Reference<Model::Source>> override {
    return sources;
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
  Sources(
      Perimortem::Memory::Allocator::Arena& arena,
      Perimortem::Core::View::Vector<Ttx::Concept::Reference<Model::Source>>
          members,
      const Model::Namespace& exports);

  auto collect_namespace(const Model::Namespace& namespace_object) -> void;
  auto collect_definition(const Ttx::Concept::Abstract& definition) -> void;

  using SourceIndex =
      Perimortem::Memory::Managed::Map<const Model::Source*, Bool>;
  using DefinitionIndex =
      Perimortem::Memory::Managed::Map<const Ttx::Concept::Abstract*, Count>;

  const Model::Namespace& exports;
  Perimortem::Memory::Managed::Vector<Ttx::Concept::Reference<Model::Source>>
      sources;
  SourceIndex source_index;
  Perimortem::Memory::Managed::Vector<Ttx::Concept::Reference<Model::Package>>
      dependencies;
  Perimortem::Memory::Managed::Vector<
      Ttx::Concept::Reference<Ttx::Concept::Abstract>>
      definitions;
  DefinitionIndex definition_index;
  Perimortem::Memory::Managed::Vector<Ttx::Concept::Reference<Model::Namespace>>
      pending_namespaces;
  Bool valid = True;
};

}  // namespace Tetrodotoxin::Model::Packages
