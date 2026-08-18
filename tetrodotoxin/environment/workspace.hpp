// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/option.hpp"

#include "perimortem/memory/allocator/arena.hpp"
#include "perimortem/memory/dynamic/object.hpp"
#include "perimortem/memory/dynamic/vector.hpp"
#include "perimortem/memory/managed/map.hpp"
#include "perimortem/memory/managed/vector.hpp"

#include "perimortem/system/version.hpp"

#include "tetrodotoxin/environment/dialects.hpp"
#include "tetrodotoxin/package/language/monograph.hpp"
#include "ttx/lexical/associations.hpp"
#include "ttx/lexical/errors.hpp"

namespace Tetrodotoxin::Environment {

// Owns one semantic island and every Monograph published into it. Direct
// sources and fixed Package source tables complete atomically here.
class Workspace : public Ttx::Concept::Abstract {
 public:
  Workspace();
  ~Workspace() override;

  // Installs a distinct stateful Dialect under one exact authored name.
  template <typename TargetDialect, typename... DependencyDialects>
  auto install_dialect(
      Perimortem::Core::View::Bytes name,
      DependencyDialects&... dependencies) -> TargetDialect* {
    return dialects.install<TargetDialect>(name, dependencies...);
  }

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

  // Returns the immutable authored source index published with one completed
  // Monograph. The borrowed identities share the retained source transaction.
  auto get_associations(const Language::Monograph& monograph) const
      -> Perimortem::Core::Option<const Ttx::Lexical::Associations&>;

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
    Perimortem::Memory::Dynamic::Object<Perimortem::Memory::Allocator::Arena>
        transaction;
    Language::Monograph& monograph;
    const Ttx::Lexical::Associations& associations;
  };

  // Declaration order is lifetime order. Reverse destruction releases
  // retained Monographs before their installed Dialects.
  Perimortem::Memory::Allocator::Arena arena;
  Dialects dialects;
  Perimortem::Memory::Dynamic::Vector<PublishedSource> published_sources;
  Perimortem::Memory::Managed::
      Map<Perimortem::Core::View::Bytes, Language::Monograph&>
          source_monographs;
  Perimortem::Memory::Managed::Vector<ImportedPackage> packages;
};

}  // namespace Tetrodotoxin::Environment
