// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/option.hpp"

#include "perimortem/memory/allocator/arena.hpp"
#include "perimortem/memory/dynamic/record.hpp"
#include "perimortem/memory/dynamic/vector.hpp"
#include "perimortem/memory/managed/map.hpp"
#include "perimortem/memory/managed/vector.hpp"

#include "perimortem/system/version.hpp"

#include "tetrodotoxin/environment/toolchain.hpp"
#include "tetrodotoxin/package/archive/archive.hpp"
#include "tetrodotoxin/package/language/monograph.hpp"
#include "tetrodotoxin/package/snapshots.hpp"
#include "ttx/lexical/associations.hpp"
#include "ttx/lexical/errors.hpp"

namespace Tetrodotoxin::Environment {

// Owns one semantic island and every Monograph published into it. Direct
// sources and fixed Package source tables complete atomically here.
class Workspace : public Ttx::Concept::Abstract {
 public:
  class AuthoredLocation {
   public:
    constexpr AuthoredLocation(
        Perimortem::Core::View::Bytes package_root,
        Perimortem::Core::View::Bytes diagnostic_path,
        Perimortem::Core::View::Bytes source_text,
        Ttx::Lexical::Anchor anchor)
        : package_root(package_root),
          diagnostic_path(diagnostic_path),
          source_text(source_text),
          anchor(anchor) {}

    constexpr auto get_package_root() const -> Perimortem::Core::View::Bytes {
      return package_root;
    }

    constexpr auto get_diagnostic_path() const
        -> Perimortem::Core::View::Bytes {
      return diagnostic_path;
    }

    constexpr auto get_source_text() const -> Perimortem::Core::View::Bytes {
      return source_text;
    }

    constexpr auto get_anchor() const -> Ttx::Lexical::Anchor { return anchor; }

   private:
    Perimortem::Core::View::Bytes package_root;
    Perimortem::Core::View::Bytes diagnostic_path;
    Perimortem::Core::View::Bytes source_text;
    Ttx::Lexical::Anchor anchor;
  };

  Workspace(
      Toolchain& toolchain,
      Perimortem::Core::Option<
          Perimortem::Memory::Dynamic::Record<Package::Snapshots>> snapshots =
          {});
  ~Workspace() override;

  // Completes one direct source transaction before publishing its semantic
  // name. Parse, link, or finalization failure publishes nothing.
  auto interpret_source(
      Ttx::Lexical::Errors& errors,
      Perimortem::Core::View::Bytes semantic_name,
      Perimortem::Core::View::Bytes diagnostic_path,
      Perimortem::Core::View::Bytes contents)
      -> Perimortem::Core::Option<Language::Monograph&>;

  // Interprets one Package manifest and exactly the Sources described by that
  // manifest. Dependencies must already be completed in this Workspace.
  // importing a Package never starts another import.
  auto import_package(
      Ttx::Lexical::Errors& errors,
      Perimortem::Core::View::Bytes package_root,
      Perimortem::Core::View::Bytes root_semantic_name,
      Perimortem::Core::View::Bytes root_logical_route,
      Perimortem::Core::View::Bytes root_package_identity,
      Perimortem::System::Version root_package_version)
      -> Perimortem::Core::Option<Language::Monograph&>;

  // Reconstructs one source-free Package from validated Archive facts. Every
  // dependency must already be restored in this Workspace.
  auto restore_package(
      const Package::Archive::Archive& archive,
      Perimortem::Core::View::Bytes root_semantic_name)
      -> Perimortem::Core::Option<Language::Monograph&>;

  // Returns the immutable authored source index published with one completed
  // Monograph. The borrowed identities share the retained source transaction.
  auto get_associations(const Language::Monograph& monograph) const
      -> Perimortem::Core::Option<const Ttx::Lexical::Associations&>;

  auto get_associations(Perimortem::Core::View::Bytes diagnostic_path) const
      -> Perimortem::Core::Option<const Ttx::Lexical::Associations&>;

  auto find_authored_location(const Ttx::Concept::Abstract& semantic) const
      -> Perimortem::Core::Option<AuthoredLocation>;

  auto get_name() const -> Perimortem::Core::View::Bytes override;
  auto get_documentation() const -> const Ttx::Concept::Documentation& override;
  auto resolve() const -> const Ttx::Concept::Abstract& override;
  auto resolve_context(Perimortem::Core::View::Bytes route) const
      -> const Ttx::Concept::Abstract& override;

 private:
  struct ImportedPackage {
    Perimortem::Core::View::Bytes identity;
    Perimortem::System::Version version;
    Package::Language::Monograph* monograph;
  };

  // One committed source record keeps its transaction alive and publishes its
  // immutable authored source index. The operation Cursor is not retained.
  struct PublishedSource {
    Perimortem::Core::View::Bytes package_root;
    Perimortem::Core::View::Bytes diagnostic_path;
    Perimortem::Core::View::Bytes source_text;
    Perimortem::Memory::Dynamic::Record<Perimortem::Memory::Allocator::Arena>
        transaction;
    Language::Monograph& monograph;
    const Ttx::Lexical::Associations& associations;
  };

  // Toolchain outlives every Workspace and therefore every Monograph that
  // retains one of its stateless Dialect identities.
  Toolchain& toolchain;
  Perimortem::Core::Option<
      Perimortem::Memory::Dynamic::Record<Package::Snapshots>>
      snapshots;
  Perimortem::Memory::Allocator::Arena arena;
  Perimortem::Memory::Dynamic::Vector<PublishedSource> published_sources;
  Perimortem::Memory::Dynamic::Vector<
      Perimortem::Memory::Dynamic::Record<Perimortem::Memory::Allocator::Arena>>
      restored_transactions;
  Perimortem::Memory::Managed::
      Map<Perimortem::Core::View::Bytes, Language::Monograph&>
          source_monographs;
  Perimortem::Memory::Managed::Vector<ImportedPackage> packages;
};

}  // namespace Tetrodotoxin::Environment
