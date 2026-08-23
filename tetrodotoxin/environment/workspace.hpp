// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/view/vector.hpp"
#include "perimortem/core/option.hpp"

#include "perimortem/memory/allocator/arena.hpp"
#include "perimortem/memory/dynamic/map.hpp"
#include "perimortem/memory/dynamic/record.hpp"
#include "perimortem/memory/dynamic/vector.hpp"
#include "perimortem/memory/managed/vector.hpp"

#include "perimortem/system/version.hpp"

#include "tetrodotoxin/environment/toolchain.hpp"
#include "tetrodotoxin/package/archive/archive.hpp"
#include "tetrodotoxin/package/language/monograph.hpp"
#include "tetrodotoxin/package/snapshots.hpp"
#include "ttx/lexical/associations.hpp"
#include "ttx/lexical/errors.hpp"
#include "ttx/lexical/token.hpp"

namespace Tetrodotoxin::Environment {

// A Workspace gathers every Monograph that can refer to one another. It keeps
// incomplete source transactions available to tooling while completed islands
// remain the only inputs admitted to Terminal production.
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

  // A direct source retains its transaction once its Dialect creates a
  // Monograph. The optional result still reports full semantic completion, so
  // build callers and editor callers can share one operation safely.
  auto interpret_source(
      Ttx::Lexical::Errors& errors,
      Perimortem::Core::View::Bytes semantic_name,
      Perimortem::Core::View::Bytes diagnostic_path,
      Perimortem::Core::View::Bytes contents)
      -> Perimortem::Core::Option<Language::Monograph&>;

  // A Package begins with its manifest because that table gives every member a
  // stable name and path. Dependencies arrive as completed Workspace facts, so
  // this import cannot quietly start another import.
  auto import_package(
      Ttx::Lexical::Errors& errors,
      Perimortem::Core::View::Bytes package_root,
      Perimortem::Core::View::Bytes root_semantic_name,
      Perimortem::Core::View::Bytes root_logical_route,
      Perimortem::Core::View::Bytes root_package_identity,
      Perimortem::System::Version root_package_version)
      -> Perimortem::Core::Option<Language::Monograph&>;

  // Archive restoration rebuilds a Package from facts that have already passed
  // archive validation. Completing dependencies first gives restored members
  // the same context as authored members.
  auto restore_package(
      const Package::Archive::Archive& archive,
      Perimortem::Core::View::Bytes root_semantic_name)
      -> Perimortem::Core::Option<Language::Monograph&>;

  // Each retained Monograph keeps the authored index created in its source
  // transaction. Tooling can borrow the strongest identities interpretation
  // established even when later completion reports an error.
  auto get_associations(const Language::Monograph& monograph) const
      -> Perimortem::Core::Option<const Ttx::Lexical::Associations&>;

  auto get_associations(Perimortem::Core::View::Bytes diagnostic_path) const
      -> Perimortem::Core::Option<const Ttx::Lexical::Associations&>;

  auto get_monograph(Perimortem::Core::View::Bytes diagnostic_path) const
      -> Perimortem::Core::Option<const Language::Monograph&>;

  auto get_completed_monograph(Perimortem::Core::View::Bytes diagnostic_path)
      const -> Perimortem::Core::Option<const Language::Monograph&>;

  // Keeping the original Token stream beside a retained source lets tooling
  // borrow the same lexical facts that built its semantic graph. That shared
  // view saves another tokenization pass and keeps source coordinates aligned.
  auto get_tokens(Perimortem::Core::View::Bytes diagnostic_path) const
      -> Perimortem::Core::View::Vector<Ttx::Lexical::Token>;

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

  // One retained source keeps its text, Tokens, authored index, semantic root,
  // and Arena together. Completion decides product eligibility without
  // discarding the evidence an editor can still use.
  struct RetainedSource {
    Perimortem::Core::View::Bytes package_root;
    Perimortem::Core::View::Bytes diagnostic_path;
    Perimortem::Core::View::Bytes source_text;
    Perimortem::Memory::Dynamic::Record<Perimortem::Memory::Allocator::Arena>
        transaction;
    Language::Monograph& monograph;
    Perimortem::Core::View::Vector<Ttx::Lexical::Token> tokens;
    const Ttx::Lexical::Associations& associations;
    Bool completed;
  };

  // Workspace borrows one Toolchain for its full lifetime. Monographs can then
  // keep the exact installed Dialect identities without owning another
  // registry.
  Toolchain& toolchain;
  Perimortem::Core::Option<
      Perimortem::Memory::Dynamic::Record<Package::Snapshots>>
      snapshots;
  Perimortem::Memory::Allocator::Arena arena;
  Perimortem::Memory::Dynamic::Vector<RetainedSource> retained_sources;
  Perimortem::Memory::Dynamic::Vector<
      Perimortem::Memory::Dynamic::Record<Perimortem::Memory::Allocator::Arena>>
      restored_transactions;
  Perimortem::Memory::Dynamic::Map<
      Perimortem::Core::View::Bytes,
      Ttx::Concept::Reference<Language::Monograph>>
      retained_monographs;
  Perimortem::Memory::Managed::Vector<ImportedPackage> packages;
};

}  // namespace Tetrodotoxin::Environment
