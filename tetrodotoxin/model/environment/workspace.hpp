// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/static/vector.hpp"

#include "perimortem/memory/allocator/arena.hpp"
#include "perimortem/memory/managed/map.hpp"
#include "perimortem/memory/managed/vector.hpp"

#include "perimortem/system/version.hpp"

#include "tetrodotoxin/model/environment/dependency.hpp"
#include "ttx/concept/reference.hpp"
#include "ttx/model/types/generics/access.hpp"
#include "ttx/model/types/generics/fixed.hpp"
#include "ttx/model/types/generics/view.hpp"

namespace Tetrodotoxin::Model::Environment {

// Workspace is the resolved container supplied to every Source in one
// interpretation transaction. Package construction selects authored names and
// exact versions before evaluation, then records those completed facts here.
// Source lookup can therefore bind a local Alias without searching a
// repository or rediscovering a transitive package graph.
//
// The Workspace owns every injected Alias, the common Generic formulas, and
// the append only materializations shared by the transaction. Materialized
// Types therefore have one address across every member Source without relying
// on process static storage. A closed compile time name table selects the
// immutable formula objects while imported and host bindings remain in the
// separate dynamic map.
//
// Package resolutions additionally retain their exact external identity and
// deduplicated Package edge. Direct bindings let a container expose already
// built local or host objects without pretending that a Source imported them.
// The Workspace and its Sources remain on one worker for the complete
// transaction because the shared arena is backed by worker local Bibliotheca
// storage.
//
// The package container transaction keeps Workspace alive while all member
// Sources exist. Package, Writer, and Compiler queries backed by Sources finish
// first. Materialization queries then must stop. At that point member Sources
// may be destroyed with the Workspace being the last root. This order keeps
// every retained formula and every Type argument owned by a Source alive
// through the writer's last query while letting the Workspace delegate arena
// ownership to individual Sources.
class Workspace {
 public:
  Workspace()
      : materializations(arena),
        generic_formulas({access, fixed, view}),
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
      const Package::Resolved& package,
      const Ttx::Concept::Documentation& documentation) -> Bool;

  auto bind(
      Perimortem::Core::View::Bytes local_name,
      const Ttx::Concept::Abstract& target,
      const Ttx::Concept::Documentation& documentation) -> Bool;

  auto resolve_context(Perimortem::Core::View::Bytes route) const
      -> const Ttx::Concept::Abstract&;

  constexpr auto get_materializations()
      -> Ttx::Model::Types::Generic::Materializations& {
    return materializations;
  }

  constexpr auto get_resolutions() const -> Perimortem::Core::View::Vector<
      Ttx::Concept::Reference<Dependencies::Package>> {
    return resolutions;
  }

  // Dependencies and Packages are parallel, deduplicated views in authored
  // first resolution order. Manifest construction consumes the identity edge.
  // Model::Package::Resolved consumes the anonymous Package at the same index.
  constexpr auto get_dependencies() const -> Perimortem::Core::View::Vector<
      Ttx::Concept::Reference<Dependencies::Package>> {
    return dependencies;
  }

  constexpr auto get_packages() const -> Perimortem::Core::View::Vector<
      Ttx::Concept::Reference<Package::Resolved>> {
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
  using Formula = Ttx::Concept::Reference<Ttx::Concept::Abstract>;
  using Formulas = Perimortem::Core::Static::Vector<Formula, 3>;

  Perimortem::Memory::Allocator::Arena arena;
  Ttx::Model::Types::Generics::View view;
  Ttx::Model::Types::Generics::Access access;
  Ttx::Model::Types::Generics::Fixed fixed;
  Ttx::Model::Types::Generic::Materializations materializations;
  // Slot order is Access, Fixed, View and matches the closed lookup table.
  Formulas generic_formulas;
  Perimortem::Memory::Managed::Vector<
      Ttx::Concept::Reference<Dependencies::Package>>
      resolutions;
  Perimortem::Memory::Managed::Vector<
      Ttx::Concept::Reference<Dependencies::Package>>
      dependencies;
  Perimortem::Memory::Managed::Vector<
      Ttx::Concept::Reference<Package::Resolved>>
      packages;
  Bindings bindings_by_name;
  Perimortem::Memory::Managed::Map<Identity, Count> dependencies_by_identity;
  Perimortem::Memory::Managed::Map<const Package::Resolved*, Count>
      dependencies_by_package;
};

}  // namespace Tetrodotoxin::Model::Environment
