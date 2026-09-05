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

#include "tetrodotoxin/environment/provider.h"
#include "tetrodotoxin/environment/toolchain.hpp"
#include "tetrodotoxin/language/query.hpp"
#include "tetrodotoxin/package/archive/archive.hpp"
#include "tetrodotoxin/package/language/monograph.hpp"
#include "tetrodotoxin/package/repository/repository.hpp"
#include "tetrodotoxin/package/resource.hpp"
#include "tetrodotoxin/package/snapshots.hpp"
#include "ttx/lexical/associations.hpp"
#include "ttx/lexical/errors.hpp"
#include "ttx/lexical/token.hpp"

namespace Tetrodotoxin::Environment {

// A Workspace is the lifetime of one connected semantic island. It retains the
// source transaction behind each Monograph so graph references can cross files
// and Dialects without copying identities into a global registry.
//
// Incomplete transactions keep their strongest available Tokens,
// Associations, diagnostics, and semantic edges for editor tooling. Linking
// and finalization decide when the entire island is complete enough for a
// Terminal producer. A later editor snapshot can release this Workspace as one
// lifetime and recompute dependents against the replacement source identities.
class Workspace : public Ttx::Concept::Abstract {
 public:

  enum class ProviderSourceState { Accepted, Incomplete, Failed, Invalid };

  struct ProviderSourceObservation {
    ProviderSourceState state;
    ttx_abstract root;
    ttx_abstract error;
  };

  class PackageSource {
   public:
    constexpr PackageSource(
        Perimortem::Core::View::Bytes name,
        Perimortem::Core::View::Bytes logical_route,
        const Language::Monograph& monograph)
        : name(name), logical_route(logical_route), monograph(monograph) {}

    constexpr auto get_name() const -> Perimortem::Core::View::Bytes {
      return name;
    }
    constexpr auto get_logical_route() const -> Perimortem::Core::View::Bytes {
      return logical_route;
    }
    constexpr auto get_monograph() const -> const Language::Monograph& {
      return monograph;
    }

   private:
    // Terminal products use the first route discovered from the Package root
    // as a deterministic graph key, while authored Aliases remain local to
    // their importers instead of turning that route into a Monograph name.
    Perimortem::Core::View::Bytes name;
    Perimortem::Core::View::Bytes logical_route;
    const Language::Monograph& monograph;
  };

  // Carries one Workspace owned editor projection. Semantic definitions retain
  // their authored Anchor and source text, while an acquired file uses an
  // empty Anchor to select the beginning of that physical input.
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
          {},
      Package::Repository::Repository* repository = nullptr,
      Ttx::Concept::Abstract* outer = nullptr);
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

  // An explicitly selected portable Dialect receives the same source bytes
  // and Workspace context as a native frontend. Workspace retains the returned
  // SourceGraph before exposing its root, so source storage, provider
  // callbacks, and every borrowed identity share one lifetime.
  auto interpret_source(
      tetrodotoxin_dialect_provider provider,
      Perimortem::Core::View::Bytes semantic_name,
      Perimortem::Core::View::Bytes diagnostic_path,
      Perimortem::Core::View::Bytes contents) -> ProviderSourceObservation;

  // Default production follows the retained source graph that owns the root.
  // Native and foreign frontends therefore share one Puffer path, while each
  // graph keeps the language-specific operation and its supporting lifetime.
  auto produce(
      ttx_abstract root,
      ttx_context context,
      Perimortem::Memory::Allocator::Arena& result_arena,
      tetrodotoxin_production_result result) const -> Bool;

  auto produce(
      ttx_abstract root,
      ttx_context context,
      Perimortem::Memory::Allocator::Arena& result_arena) const
      -> Tetrodotoxin::Language::ProductionObservation;

  // A Package begins with its restricted export source. Workspace follows
  // external Types relative to each importer and terminates at
  // exact Package identities already supplied by the repository boundary.
  auto import_package(
      Ttx::Lexical::Errors& errors,
      Perimortem::Core::View::Bytes package_root,
      Perimortem::Core::View::Bytes root_semantic_name,
      Perimortem::Core::View::Bytes root_logical_route)
      -> Perimortem::Core::Option<Language::Monograph&>;

  // Archive restoration rebuilds the recorded Package graph after its bytes
  // pass archive validation, then applies the same completion order as authored
  // sources.
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

  auto get_associations(
      Perimortem::Core::View::Bytes package_root,
      Perimortem::Core::View::Bytes logical_route) const
      -> Perimortem::Core::Option<const Ttx::Lexical::Associations&>;

  auto get_monograph(Perimortem::Core::View::Bytes diagnostic_path) const
      -> Perimortem::Core::Option<const Language::Monograph&>;

  auto get_monograph(
      Perimortem::Core::View::Bytes package_root,
      Perimortem::Core::View::Bytes logical_route) const
      -> Perimortem::Core::Option<const Language::Monograph&>;

  auto get_completed_monograph(Perimortem::Core::View::Bytes diagnostic_path)
      const -> Perimortem::Core::Option<const Language::Monograph&>;

  auto get_completed_monograph(
      Perimortem::Core::View::Bytes package_root,
      Perimortem::Core::View::Bytes logical_route) const
      -> Perimortem::Core::Option<const Language::Monograph&>;

  auto get_package_source_count(
      const Package::Language::Monograph& package) const -> Count;

  auto get_package_source(
      const Package::Language::Monograph& package,
      Count index) const -> Perimortem::Core::Option<PackageSource>;

  // A Terminal that has already proved the local Workspace contract can trace
  // an exported identity back to the Package whose source edge introduced it.
  // The relationship comes from Workspace's retained acquisition graph rather
  // than a package name search or a compiler-owned member index.
  auto find_package_owner(const Ttx::Concept::Abstract& semantic) const
      -> Perimortem::Core::Option<const Package::Language::Monograph&>;

  // Environment may recover the invocation that owns a bootstrap Workspace
  // after the public Workspace relationship has been proved. Other consumers
  // continue through concept queries and never use this local lifetime edge.
  constexpr auto get_outer() const -> Ttx::Concept::Abstract* { return outer; }

  // Terminal production receives this typed support view after Environment has
  // admitted the provider. The view keeps Workspace representation private
  // while lending the graph root and revision records needed to seal outputs.
  auto get_provider_handle() const -> tetrodotoxin_workspace_view;

  // A Terminal may need the authored bytes only to attach diagnostics to the
  // semantic owner it is projecting. This lookup returns the retained source
  // transaction for a live Monograph; a restored package correctly has no
  // authored input in this Workspace.
  auto find_source_input(const Language::Monograph& monograph) const
      -> Perimortem::Core::Option<AuthoredLocation>;

  // Keeping the original Token stream beside a retained source lets tooling
  // borrow the same Tokens and source ranges that built its semantic graph.
  // That shared view saves another tokenization pass and keeps source
  // coordinates aligned.
  auto get_tokens(Perimortem::Core::View::Bytes diagnostic_path) const
      -> Perimortem::Core::View::Vector<Ttx::Lexical::Token>;

  auto get_tokens(
      Perimortem::Core::View::Bytes package_root,
      Perimortem::Core::View::Bytes logical_route) const
      -> Perimortem::Core::View::Vector<Ttx::Lexical::Token>;

  auto find_authored_location(const Ttx::Concept::Abstract& semantic) const
      -> Perimortem::Core::Option<AuthoredLocation>;

  // Tooling may project a selected locator back to the physical input that
  // satisfied it. This query follows the retained acquisition relationship and
  // does not add source paths to Import or Resource semantic identity.
  auto find_acquired_location(
      Perimortem::Core::View::Bytes package_root,
      Perimortem::Core::View::Bytes logical_route,
      Count offset,
      const Ttx::Concept::Abstract& semantic) const
      -> Perimortem::Core::Option<AuthoredLocation>;

  auto get_name() const -> Perimortem::Core::View::Bytes override;
  auto get_documentation() const -> const Ttx::Concept::Documentation& override;
  auto resolve() const -> const Ttx::Concept::Abstract& override;
  auto resolve_concept(Perimortem::Core::View::Bytes route) const
      -> const Ttx::Concept::Abstract& override;
  auto visit_concepts(ttx_named_abstract_callable* visitor) const
      -> void override;

  auto resolve_concept(ttx_borrowed_bytes route) const -> ttx_abstract override;
  void visit_concepts(ttx_concept_sink result) const override;

 protected:
  auto negotiate(ttx_abstract requirement) const
      -> ttx_interface_relation override;

 private:
  static auto select(tetrodotoxin_workspace_view_self* self) -> Workspace&;
  static auto TTX_CALL provider_root(tetrodotoxin_workspace_view_self* self)
      -> ttx_abstract;
  static void TTX_CALL provider_revisions(
      tetrodotoxin_workspace_view_self* self,
      tetrodotoxin_closure_authority_sink result);
  static const tetrodotoxin_workspace_view_ops provider_operations;

  struct ImportedPackage {
    Perimortem::Core::View::Bytes identity;
    Perimortem::System::Version version;
    Package::Language::Monograph* monograph;
  };

  struct PackageMember {
    const Package::Language::Monograph* package;
    Perimortem::Core::View::Bytes name;
    Perimortem::Core::View::Bytes logical_route;
    const Language::Monograph* monograph;
  };

  struct ActivePackage {
    Perimortem::Core::View::Bytes identity;
    Perimortem::System::Version version;
  };

  struct RetainedGraph {
    ttx_abstract root;
    Language::Monograph* local;
  };

  struct SourceRevision {
    auto handle() -> tetrodotoxin_authority_revision;

   private:
    static auto select(tetrodotoxin_authority_revision_self* self)
        -> SourceRevision&;
    static auto TTX_CALL revision(tetrodotoxin_authority_revision_self* self)
        -> uint64_t;
    static auto TTX_CALL is_current(tetrodotoxin_authority_revision_self* self)
        -> uint8_t;
    static const tetrodotoxin_authority_revision_ops operations;

  };

  // A provider may borrow source bytes and its own operation tables for every
  // later graph query. Keeping the source transaction beside that graph and
  // releasing the handle first preserves both dependencies without wrapping
  // the returned semantic identity in a C++ object.
  struct ProviderSource {
    ProviderSource(
        Perimortem::Memory::Dynamic::Record<
            Perimortem::Memory::Allocator::Arena> transaction,
        tetrodotoxin_source_graph graph)
        : transaction(transaction), graph(graph) {}

    ~ProviderSource() { graph.operations->release(graph.self); }

    ProviderSource(const ProviderSource&) = delete;
    ProviderSource(ProviderSource&&) = delete;
    auto operator=(const ProviderSource&) -> ProviderSource& = delete;
    auto operator=(ProviderSource&&) -> ProviderSource& = delete;

    Perimortem::Memory::Dynamic::Record<Perimortem::Memory::Allocator::Arena>
        transaction;
    tetrodotoxin_source_graph graph;
    SourceRevision revision;
  };

  // One retained source keeps its text, Tokens, authored index, semantic root,
  // and Arena together. Completion decides product eligibility without
  // discarding the evidence an editor can still use.
  struct RetainedSource {
    Perimortem::Core::View::Bytes package_root;
    Perimortem::Core::View::Bytes name;
    Perimortem::Core::View::Bytes logical_route;
    Perimortem::Core::View::Bytes diagnostic_path;
    Perimortem::Core::View::Bytes source_text;
    Perimortem::Memory::Dynamic::Record<Perimortem::Memory::Allocator::Arena>
        transaction;
    Language::Monograph& monograph;
    Perimortem::Core::View::Vector<Ttx::Lexical::Token> tokens;
    const Ttx::Lexical::Associations& associations;
    Bool completed;
    SourceRevision revision;
  };

  auto find_retained_source(
      Perimortem::Core::View::Bytes package_root,
      Perimortem::Core::View::Bytes logical_route) const
      -> const RetainedSource*;

  auto restore_coordinate(
      Ttx::Lexical::Errors& errors,
      Perimortem::Core::View::Bytes identity,
      Perimortem::System::Version version) -> Bool;

  // Workspace borrows one Toolchain for its full lifetime. Monographs can then
  // keep the exact installed Dialect identities without owning another
  // registry.
  Toolchain& toolchain;
  Package::Repository::Repository* repository;
  Ttx::Concept::Abstract* outer;
  Perimortem::Core::Option<
      Perimortem::Memory::Dynamic::Record<Package::Snapshots>>
      snapshots;
  Perimortem::Memory::Allocator::Arena arena;
  Perimortem::Memory::Dynamic::Vector<RetainedSource> retained_sources;
  Perimortem::Memory::Dynamic::Vector<
      Perimortem::Memory::Dynamic::Record<ProviderSource>>
      provider_sources;
  Perimortem::Memory::Managed::Vector<PackageMember> package_members;
  Perimortem::Memory::Dynamic::Vector<
      Perimortem::Memory::Dynamic::Record<Perimortem::Memory::Allocator::Arena>>
      restored_transactions;
  Perimortem::Memory::Dynamic::Map<Perimortem::Core::View::Bytes, RetainedGraph>
      retained_graphs;
  Perimortem::Memory::Managed::Vector<ImportedPackage> packages;
  Perimortem::Memory::Dynamic::Vector<ActivePackage> active_packages;
};

}  // namespace Tetrodotoxin::Environment
