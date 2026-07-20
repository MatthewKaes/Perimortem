// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/memory/allocator/arena.hpp"
#include "perimortem/memory/managed/vector.hpp"

#include "tetrodotoxin/model/namespace.hpp"
#include "tetrodotoxin/model/package.hpp"

namespace Tetrodotoxin::Model::Packages {

// Precompiled is the Package graph reconstructed from a Puffer Buffer. The
// Archiver owns how records become real Abstract objects. This class exposes
// the restored public graph without pretending the original Sources survived.
class Precompiled final : public Model::Package {
 public:
  Precompiled(
      Perimortem::Memory::Allocator::Arena& arena,
      const Model::Namespace& exports,
      Perimortem::Core::View::Vector<Ttx::Concept::Reference<Model::Package>>
          dependencies = {});

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

  auto resolve_context(Perimortem::Core::View::Bytes route) const
      -> const Ttx::Concept::Abstract& override;

 private:
  const Model::Namespace& exports;
  Perimortem::Memory::Managed::Vector<Ttx::Concept::Reference<Model::Package>>
      dependencies;
};

}  // namespace Tetrodotoxin::Model::Packages
