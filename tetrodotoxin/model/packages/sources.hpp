// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/memory/allocator/arena.hpp"
#include "perimortem/memory/managed/vector.hpp"

#include "tetrodotoxin/model/namespace.hpp"
#include "tetrodotoxin/model/packages/interpreted.hpp"

namespace Tetrodotoxin::Model::Packages {

// Sources is Tetrodotoxin's concrete Package assembled from an already
// resolved Source graph. It retains the complete Source closure for tooling and
// the distinct Package dependencies for consumers. It performs no locator or
// name resolution: those facts have already become typed Dependency edges on
// each Source.
//
// Interpreted remains the capability, so hosts can provide alternative
// source backed Package implementations without changing Package consumers.
class Sources final : public Interpreted {
 public:
  Sources(
      Perimortem::Memory::Allocator::Arena& arena,
      const Model::Source& source,
      const Model::Namespace& exports);

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

  auto resolve_context(Perimortem::Core::View::Bytes route) const
      -> const Ttx::Concept::Abstract& override;

 private:
  auto collect_source(const Model::Source& source) -> void;

  const Model::Namespace& exports;
  Perimortem::Memory::Managed::Vector<Ttx::Concept::Reference<Model::Source>>
      sources;
  Perimortem::Memory::Managed::Vector<Ttx::Concept::Reference<Model::Package>>
      dependencies;
};

}  // namespace Tetrodotoxin::Model::Packages
