// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/memory/allocator/arena.hpp"
#include "perimortem/memory/managed/map.hpp"

#include "perimortem/system/version.hpp"

#include "perimortem/utility/option.hpp"

#include "tetrodotoxin/environment/dialects.hpp"
#include "tetrodotoxin/environment/resolution.hpp"
#include "tetrodotoxin/environment/retention.hpp"
#include "tetrodotoxin/package/repository/repository.hpp"

namespace Tetrodotoxin::Environment {

// Owns one semantic island and composes the Environment state needed to import
// it. Workspace keeps public orchestration while Dialects, Retention, and
// Resolution own their narrower lifetime and transaction invariants.
class Workspace : public Ttx::Concept::Abstract {
 public:
  Workspace();
  ~Workspace() override;

  // Installs a distinct stateful Dialect under one exact authored name.
  template <typename TargetDialect>
  auto install_dialect(Perimortem::Core::View::Bytes name) -> Bool {
    return dialects.install<TargetDialect>(name);
  }

  // Imports caller owned source bytes and publishes the resulting Monograph
  // under its exact Workspace global semantic name.
  auto import_source(
      Ttx::Lexical::Errors& errors,
      Perimortem::Core::View::Bytes semantic_name,
      Perimortem::Core::View::Bytes diagnostic_path,
      Perimortem::Core::View::Bytes contents)
      -> Perimortem::Utility::Option<Language::Dialect::Monograph&>;

  // Imports one confined Package island and resolves exact semantic Archives
  // from the explicit Repository. Source staging failure returns no result,
  // while Resolution returns either the completed root or a failure category.
  auto import_package(
      Ttx::Lexical::Errors& errors,
      Perimortem::Core::View::Bytes package_root,
      Perimortem::Core::View::Bytes root_semantic_name,
      Perimortem::Core::View::Bytes root_logical_route,
      Perimortem::Core::View::Bytes root_package_identity,
      Perimortem::System::Version root_package_version,
      Package::Repository::Repository& repository) -> Perimortem::Core::Static::
      Union<Language::Dialect::Monograph&, Package::Repository::SelectionError>;

  auto get_name() const -> Perimortem::Core::View::Bytes override;
  auto get_documentation() const -> const Ttx::Concept::Documentation& override;
  auto resolve() const -> const Ttx::Concept::Abstract& override;
  auto resolve_context(Perimortem::Core::View::Bytes route) const
      -> const Ttx::Concept::Abstract& override;

 private:
  // Package Storage already uses this Arena. Its retained path and body views
  // can therefore enter the same source transaction without another copy.
  auto import_retained_source(
      Ttx::Lexical::Errors& errors,
      Perimortem::Core::View::Bytes semantic_name,
      Perimortem::Core::View::Bytes diagnostic_path,
      Perimortem::Core::View::Bytes contents,
      Bool publish_globally)
      -> Perimortem::Utility::Option<Language::Dialect::Monograph&>;

  // Declaration order is lifetime order. Reverse destruction releases
  // Resolution first, then Monographs, then their host Dialects, then Arena
  // pages after every borrowing object is gone.
  Perimortem::Memory::Allocator::Arena arena;
  Dialects dialects;
  Retention retention;
  Resolution resolution;
  Perimortem::Memory::Managed::
      Map<Perimortem::Core::View::Bytes, Language::Dialect::Monograph&>
          source_monographs;
};

}  // namespace Tetrodotoxin::Environment
