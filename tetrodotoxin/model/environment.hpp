// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/memory/allocator/arena.hpp"
#include "perimortem/memory/managed/map.hpp"
#include "perimortem/memory/managed/vector.hpp"

#include "perimortem/system/version.hpp"

#include "tetrodotoxin/model/dependencies/package.hpp"
#include "ttx/concept/reference.hpp"
#include "ttx/model/types/generics/access.hpp"
#include "ttx/model/types/generics/view.hpp"

namespace Tetrodotoxin::Model {

// Environment is the resolved container supplied to every Source in one
// interpretation transaction. Puffer selects authored package names and exact
// versions before evaluation, then records those completed facts here. Source
// lookup can therefore bind a local Alias without searching a repository or
// rediscovering a transitive package graph.
//
// The Environment owns every injected Alias and the common Generic formulas
// shared by the transaction. Their materializations therefore have one address
// across every member Source without relying on process-static storage. Package
// resolutions additionally retain their exact external identity and
// deduplicated Package edge. Direct bindings let a container expose
// already-built local or host objects without pretending that a Source imported
// them. The Environment and its Sources remain on one worker for the complete
// transaction because the shared arena is backed by worker-local Bibliotheca
// storage.
class Environment {
 public:
  Environment()
      : view(arena),
        access(arena),
        resolutions(arena),
        dependencies(arena),
        packages(arena),
        bindings_by_name(arena),
        dependencies_by_identity(arena),
        dependencies_by_package(arena) {}

  auto resolve(
      const Dialect& root_dialect,
      Perimortem::Core::View::Bytes local_name,
      Perimortem::Core::View::Bytes package_name,
      Perimortem::System::Version version,
      const Package& package,
      const Ttx::Concept::Documentation& documentation) -> Bool;

  auto bind(
      Perimortem::Core::View::Bytes local_name,
      const Ttx::Concept::Abstract& target,
      const Ttx::Concept::Documentation& documentation) -> Bool;

  auto resolve_context(Perimortem::Core::View::Bytes route) const
      -> const Ttx::Concept::Abstract&;

  constexpr auto get_resolutions() const -> Perimortem::Core::View::Vector<
      Ttx::Concept::Reference<Dependencies::Package>> {
    return resolutions;
  }

  // Dependencies and Packages are parallel, deduplicated views in authored
  // first resolution order. Manifest construction consumes the identity edge.
  // Model::Package consumes the anonymous Package at the same index.
  constexpr auto get_dependencies() const -> Perimortem::Core::View::Vector<
      Ttx::Concept::Reference<Dependencies::Package>> {
    return dependencies;
  }

  constexpr auto get_packages() const
      -> Perimortem::Core::View::Vector<Ttx::Concept::Reference<Package>> {
    return packages;
  }

 private:
  class Identity {
   public:
    constexpr Identity(
        Perimortem::Core::View::Bytes name,
        Perimortem::System::Version version)
        : name(name), version(version) {}

    constexpr auto operator==(const Identity& rhs) const -> Bool {
      return name == rhs.name && version == rhs.version;
    }

    constexpr auto hash() const -> Unsigned_64 {
      Unsigned_64 encoded =
          (Unsigned_64(version.get_major()) << 16) | version.get_minor();
      return Perimortem::Core::Hash(name).Rehash(encoded);
    }

   private:
    Perimortem::Core::View::Bytes name;
    Perimortem::System::Version version;
  };

  using Bindings = Perimortem::Memory::Managed::Map<
      Perimortem::Core::View::Bytes,
      Ttx::Concept::Reference<Ttx::Model::Alias>>;

  Perimortem::Memory::Allocator::Arena arena;
  Ttx::Model::Types::Generics::View view;
  Ttx::Model::Types::Generics::Access access;
  Perimortem::Memory::Managed::Vector<
      Ttx::Concept::Reference<Dependencies::Package>>
      resolutions;
  Perimortem::Memory::Managed::Vector<
      Ttx::Concept::Reference<Dependencies::Package>>
      dependencies;
  Perimortem::Memory::Managed::Vector<Ttx::Concept::Reference<Package>>
      packages;
  Bindings bindings_by_name;
  Perimortem::Memory::Managed::Map<Identity, Count> dependencies_by_identity;
  Perimortem::Memory::Managed::Map<const Package*, Count>
      dependencies_by_package;
};

}  // namespace Tetrodotoxin::Model
