// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/option.hpp"

#include "perimortem/memory/allocator/arena.hpp"
#include "perimortem/memory/managed/map.hpp"
#include "perimortem/memory/managed/vector.hpp"

#include "perimortem/system/version.hpp"

#include "perimortem/utility/result.hpp"

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

  // Interprets caller owned source bytes into the current staged range. Raw
  // lookup exposes staged identity while persistent publication waits for
  // separate link and finalize barriers.
  auto interpret_source(
      Ttx::Lexical::Errors& errors,
      Perimortem::Core::View::Bytes semantic_name,
      Perimortem::Core::View::Bytes diagnostic_path,
      Perimortem::Core::View::Bytes contents)
      -> Perimortem::Core::Option<Language::Monograph&>;

  auto link(Ttx::Lexical::Errors& errors) -> Bool;
  auto finalize(Ttx::Lexical::Errors& errors) -> Bool;
  auto abandon() -> void;

  // Imports one confined Package island and resolves exact semantic Archives
  // from the explicit Repository. Result returns the completed root or one
  // failure category. Unknown covers diagnosed staging, restoration, linking,
  // and finalization failures with no narrower Repository selection category.
  auto import_package(
      Ttx::Lexical::Errors& errors,
      Perimortem::Core::View::Bytes package_root,
      Perimortem::Core::View::Bytes root_semantic_name,
      Perimortem::Core::View::Bytes root_logical_route,
      Perimortem::Core::View::Bytes root_package_identity,
      Perimortem::System::Version root_package_version,
      Package::Repository::Repository& repository) -> Perimortem::Utility::
      Result<Language::Monograph&, Package::Repository::SelectionError>;

  auto get_name() const -> Perimortem::Core::View::Bytes override;
  auto get_documentation() const -> const Ttx::Concept::Documentation& override;
  auto resolve() const -> const Ttx::Concept::Abstract& override;
  auto resolve_context(Perimortem::Core::View::Bytes route) const
      -> const Ttx::Concept::Abstract& override;

 private:
  class StagedPublication {
   public:
    StagedPublication(
        Perimortem::Core::View::Bytes name,
        Language::Monograph& monograph);

    auto get_name() const -> Perimortem::Core::View::Bytes;
    auto get_monograph() const -> Language::Monograph&;

   private:
    Perimortem::Core::View::Bytes name;
    Language::Monograph& monograph;
  };

  // Package Storage already uses this Arena. Its retained path and body views
  // can therefore enter the same source transaction without another copy.
  auto interpret_retained_source(
      Ttx::Lexical::Errors& errors,
      Perimortem::Core::View::Bytes semantic_name,
      Perimortem::Core::View::Bytes diagnostic_path,
      Perimortem::Core::View::Bytes contents,
      Ttx::Concept::Abstract& interpretation_context,
      Bool stage_globally) -> Perimortem::Core::Option<Language::Monograph&>;

  auto has_staged_name(Perimortem::Core::View::Bytes name) const -> Bool;
  auto publish_staged() -> void;
  auto discard_staged() -> void;

  // Declaration order is lifetime order. Reverse destruction releases
  // Resolution first, then Monographs, then their host Dialects, then Arena
  // pages after every borrowing object is gone.
  Perimortem::Memory::Allocator::Arena arena;
  Dialects dialects;
  Retention retention;
  Resolution resolution;
  Perimortem::Memory::Managed::
      Map<Perimortem::Core::View::Bytes, Language::Monograph&>
          source_monographs;
  Perimortem::Memory::Managed::Vector<StagedPublication> staged_publications;
};

}  // namespace Tetrodotoxin::Environment
