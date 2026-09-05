// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/environment/workspace.hpp"

#include "perimortem/core/diagnostics/log.hpp"

#include "perimortem/memory/dynamic/record.hpp"
#include "perimortem/memory/managed/bytes.hpp"

#include "perimortem/system/path.hpp"

#include "tetrodotoxin/library/archive/reader.hpp"
#include "tetrodotoxin/library/language/model/addressable.hpp"
#include "tetrodotoxin/library/language/model/callable.hpp"
#include "tetrodotoxin/library/language/model/type.hpp"
#include "tetrodotoxin/package/content.hpp"
#include "tetrodotoxin/package/dialect.hpp"
#include "tetrodotoxin/package/resource.hpp"
#include "tetrodotoxin/package/storage.hpp"
#include "ttx/lexical/cursor.hpp"
#include "ttx/lexical/tokenizer.hpp"
#include "ttx/reference/model/layouts/fluid.hpp"
#include "ttx/reference/model/layouts/named.hpp"
#include "ttx/concept/unknown.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Perimortem::System;
using namespace Ttx::Concept;
using namespace Ttx::Lexical;
using namespace Tetrodotoxin;

const tetrodotoxin_workspace_view_ops
    Environment::Workspace::provider_operations = {
      .header =
          {
            .size = sizeof(tetrodotoxin_workspace_view_ops),
            .abi_major = TTX_ABI_MAJOR,
            .abi_minor = TTX_ABI_MINOR,
          },
      .root = provider_root,
      .visit_revisions = provider_revisions,
};

const tetrodotoxin_authority_revision_ops
    Environment::Workspace::SourceRevision::operations = {
      .header =
          {
            .size = sizeof(tetrodotoxin_authority_revision_ops),
            .abi_major = TTX_ABI_MAJOR,
            .abi_minor = TTX_ABI_MINOR,
          },
      .revision = revision,
      .is_current = is_current,
};

auto Environment::Workspace::SourceRevision::select(
    tetrodotoxin_authority_revision_self* self) -> SourceRevision& {
  return *reinterpret_cast<SourceRevision*>(self);
}

auto Environment::Workspace::SourceRevision::handle()
    -> tetrodotoxin_authority_revision {
  return {
    .operations = &operations,
    .self = reinterpret_cast<tetrodotoxin_authority_revision_self*>(this),
  };
}

auto Environment::Workspace::SourceRevision::revision(
    tetrodotoxin_authority_revision_self*) -> uint64_t {
  return 1;
}

auto Environment::Workspace::SourceRevision::is_current(
    tetrodotoxin_authority_revision_self*) -> uint8_t {
  return 1;
}

auto Environment::Workspace::select(tetrodotoxin_workspace_view_self* self)
    -> Workspace& {
  return *reinterpret_cast<Workspace*>(self);
}

auto Environment::Workspace::provider_root(
    tetrodotoxin_workspace_view_self* self) -> ttx_abstract {
  return select(self).get_handle();
}

void Environment::Workspace::provider_revisions(
    tetrodotoxin_workspace_view_self* self,
    tetrodotoxin_closure_authority_sink result) {
  Workspace& workspace = select(self);
  for (Count index = 0; index < workspace.retained_sources.get_size();
       ++index) {
    result.operations->authority(
        result.self, workspace.retained_sources[index].revision.handle());
  }
  for (Count index = 0; index < workspace.provider_sources.get_size();
       ++index) {
    result.operations->authority(
        result.self, workspace.provider_sources[index]->revision.handle());
  }
  result.operations->completed(result.self);
}

auto Environment::Workspace::get_provider_handle() const
    -> tetrodotoxin_workspace_view {
  return {
    .operations = &provider_operations,
    .self = reinterpret_cast<tetrodotoxin_workspace_view_self*>(
        const_cast<Workspace*>(this)),
  };
}

static auto append_storage_failure(
    auto& report,
    const Package::Storage::Failure& failure,
    View::Bytes subject) -> void {
  report << subject;
  failure.get_path().visit(
      [&]() { report << " has an empty or invalid confined path."_view; },
      [&](const Path& path) {
        report << " `"_view << path.get_view() << "` "_view;
        switch (failure.get_error()) {
        case Package::Storage::Failure::Error::InvalidRoute:
          report << "is not a confined logical child."_view;
          break;
        case Package::Storage::Failure::Error::Unreadable:
          report << "could not be read from the opened Package root."_view;
          break;
        default:
          report << "could not be acquired."_view;
          break;
        }
      });
}

struct PortableSourceInput {
  tetrodotoxin_source_input_ops operations;
  View::Bytes diagnostic_path;
  View::Bytes contents;
  View::Vector<Token> tokens;
};

static auto portable_source(tetrodotoxin_source_input_self* self)
    -> PortableSourceInput& {
  return *reinterpret_cast<PortableSourceInput*>(self);
}

static auto TTX_CALL portable_source_path(tetrodotoxin_source_input_self* self)
    -> ttx_borrowed_bytes {
  const View::Bytes value = portable_source(self).diagnostic_path;
  return {.data = value.get_data(), .size = value.get_size()};
}

static auto TTX_CALL portable_source_bytes(tetrodotoxin_source_input_self* self)
    -> ttx_borrowed_bytes {
  const View::Bytes value = portable_source(self).contents;
  return {.data = value.get_data(), .size = value.get_size()};
}

static void TTX_CALL portable_source_tokens(
    tetrodotoxin_source_input_self* self,
    tetrodotoxin_token_sink result) {
  if (result.operations == nullptr || result.self == nullptr ||
      result.operations->header.abi_major != TTX_ABI_MAJOR ||
      result.operations->header.size < sizeof(tetrodotoxin_token_sink_ops) ||
      result.operations->token == nullptr ||
      result.operations->completed == nullptr) {
    return;
  }
  for (const Token& token : portable_source(self).tokens) {
    result.operations->token(
        result.self, {
                       .offset = token.get_offset(),
                       .line = token.get_line(),
                       .column = token.get_column(),
                       .size = token.get_size(),
                       .code = static_cast<U8>(token.get_code().get_type()),
                     });
  }
  result.operations->completed(result.self);
}

Environment::Workspace::Workspace(
    Toolchain& selected_toolchain,
    Option<Dynamic::Record<Package::Snapshots>> selected_snapshots,
    Package::Repository::Repository* selected_repository,
    Abstract* selected_outer)
    : toolchain(selected_toolchain),
      repository(selected_repository),
      outer(selected_outer),
      snapshots(selected_snapshots),
      arena(),
      retained_sources(),
      provider_sources(),
      package_members(arena),
      retained_graphs(),
      packages(arena),
      active_packages() {}

Environment::Workspace::~Workspace() = default;

auto Environment::Workspace::restore_coordinate(
    Errors& errors,
    View::Bytes identity,
    Version version) -> Bool {
  for (const ImportedPackage& imported : packages.get_view()) {
    if (imported.identity == identity) {
      return imported.version == version;
    }
  }
  BAIL_IF(repository == nullptr);
  for (const ActivePackage& active : active_packages.get_view()) {
    BAIL_IF(active.identity == identity);
  }

  Option<View::Bytes> source_root;
  repository->select_source(identity, version)
      .visit(
          [&](View::Bytes selected) { source_root = selected; },
          [](Package::Repository::Repository::Error) {});
  if (source_root) {
    active_packages.insert(ActivePackage{identity, version});
    auto imported = import_package(
        errors, *source_root, arena.proxy(identity), "package.ttx"_view);
    active_packages.remove(active_packages.get_size() - 1);
    return Bool(imported);
  }

  Option<const Package::Archive::Archive&> selected;
  repository->select_archive(identity, version)
      .visit(
          [&](const Package::Archive::Archive& archive) { selected = archive; },
          [](Package::Repository::Repository::Error) {});
  BAIL_IF(!selected);

  active_packages.insert(ActivePackage{identity, version});
  Bool complete = True;
  for (const Package::Archive::GraphImport& import : selected->get_imports()) {
    if (import.get_kind() == Language::Import::Kind::Package) {
      complete &=
          restore_coordinate(errors, import.get_target(), import.get_version());
    }
  }
  active_packages.remove(active_packages.get_size() - 1);
  BAIL_IF(!complete);
  return Bool(restore_package(*selected, identity));
}

auto Environment::Workspace::interpret_source(
    Errors& errors,
    View::Bytes semantic_name,
    View::Bytes diagnostic_path,
    View::Bytes contents) -> Option<Language::Monograph&> {
  // Source graph objects borrow bytes and Tokens from this candidate Arena.
  // Keeping the lexical and semantic work under one handle lets any rejection
  // release the whole unfinished graph together.
  Dynamic::Record<Allocator::Arena> transaction;
  View::Bytes retained_contents = transaction->proxy(contents);
  View::Bytes retained_path = transaction->proxy(diagnostic_path);
  Tokenizer& tokenizer = transaction->construct<Tokenizer>(
      *transaction, retained_contents, retained_path);
  Associations& associations =
      transaction->construct<Associations>(*transaction);
  Cursor& cursor =
      transaction->construct<Cursor>(tokenizer, errors, associations);

  if (retained_graphs.contains(semantic_name)) {
    cursor.create_error(
        "This semantic source name is already published in the Workspace."_view,
        semantic_name);
    return {};
  }

  Count source_error_count = errors.get_size();
  auto interpretation = Language::Dialect::interpret_source(
      toolchain.get_dialects(), cursor, *this);
  if (!interpretation) {
    if (errors.get_size() == source_error_count) {
      cursor.create_error(
          "Source interpretation failed without a more specific diagnostic."_view);
    }
    return {};
  }
  Language::Monograph& monograph = *interpretation;
  Bool completed = errors.get_size() == source_error_count;

  if (monograph.is<Package::Language::Monograph>()) {
    // A Package root needs its confined path because Workspace must walk and
    // complete every external Type edge before publishing the graph.
    if (completed) {
      cursor.create_error(
          "A Package root must be completed through Workspace Package "
          "import."_view);
    }
    return {};
  }

  // The Monograph proves that the Dialect established a durable source owner.
  // Retaining its transaction here preserves Tokens, Associations, and every
  // partial identity while later barriers decide product eligibility.
  Count retained_index = retained_sources.get_size();
  View::Bytes retained_name = arena.proxy(semantic_name);
  retained_sources.insert({
    .package_root = {},
    .name = retained_name,
    .logical_route = retained_path,
    .diagnostic_path = retained_path,
    .source_text = retained_contents,
    .transaction = transaction,
    .monograph = monograph,
    .tokens = tokenizer.get_tokens(),
    .associations = associations,
    .completed = False,
  });

  // Validation can still enrich a retained graph after interpretation reports
  // an incomplete source form. The earlier report keeps publication closed
  // while editor queries gain any Types and declaration edges that did settle.
  source_error_count = errors.get_size();
  Bool linked = monograph.link(cursor);
  if (!linked && completed && errors.get_size() == source_error_count) {
    cursor.create_error(
        "Source linking failed without a more specific diagnostic."_view);
  }
  completed &= linked;

  if (completed) {
    source_error_count = errors.get_size();
    if (!monograph.finalize(cursor)) {
      if (errors.get_size() == source_error_count) {
        cursor.create_error(
            "Source finalization failed without a more specific diagnostic."_view);
      }
      completed = False;
    }
  }

  retained_graphs.insert(
      retained_name, {
                       .root = monograph.get_handle(),
                       .local = &monograph,
                     });
  retained_sources[retained_index].completed = completed;
  return completed ? Option<Language::Monograph&>(monograph)
                   : Option<Language::Monograph&>();
}

auto Environment::Workspace::interpret_source(
    tetrodotoxin_dialect_provider provider,
    View::Bytes semantic_name,
    View::Bytes diagnostic_path,
    View::Bytes contents) -> ProviderSourceObservation {
  const ProviderSourceObservation invalid = {
    .state = ProviderSourceState::Invalid,
    .root = {},
    .error = {},
  };
  if (semantic_name.is_empty() || retained_graphs.contains(semantic_name) ||
      !toolchain.is_installed(provider)) {
    return invalid;
  }

  Dynamic::Record<Allocator::Arena> transaction;
  const View::Bytes retained_contents = transaction->proxy(contents);
  const View::Bytes retained_path = transaction->proxy(diagnostic_path);
  Tokenizer& tokenizer = transaction->construct<Tokenizer>(
      *transaction, retained_contents, retained_path);
  PortableSourceInput source = {
    .operations =
        {
          .header =
              {
                .size = sizeof(tetrodotoxin_source_input_ops),
                .abi_major = TTX_ABI_MAJOR,
                .abi_minor = TTX_ABI_MINOR,
              },
          .diagnostic_path = portable_source_path,
          .bytes = portable_source_bytes,
          .visit_tokens = portable_source_tokens,
        },
    .diagnostic_path = retained_path,
    .contents = retained_contents,
    .tokens = tokenizer.get_tokens(),
  };
  const auto interpreted = Language::interpret(
      provider,
      {
        .operations = &source.operations,
        .self = reinterpret_cast<tetrodotoxin_source_input_self*>(&source),
      },
      get_handle());
  if (interpreted.state == Language::InterpretationState::Failed) {
    return {
      .state = ProviderSourceState::Failed,
      .root = {},
      .error = interpreted.error,
    };
  }
  if (interpreted.state != Language::InterpretationState::Constructed) {
    return invalid;
  }

  const ttx_abstract root =
      interpreted.graph.operations->root(interpreted.graph.self);
  const ttx_abstract dialect =
      interpreted.graph.operations->dialect(interpreted.graph.self);
  const ttx_abstract provider_candidate =
      provider.operations->candidate(provider.self);
  if (root.operations == nullptr ||
      root.operations->header.abi_major != TTX_ABI_MAJOR ||
      root.operations->header.size < sizeof(ttx_abstract_ops) ||
      dialect.operations == nullptr ||
      dialect.operations->header.abi_major != TTX_ABI_MAJOR ||
      dialect.operations->header.size < TTX_ABSTRACT_INTERFACE_PREFIX_SIZE ||
      ttx_abstract_same(root, ttx_unknown()) ||
      ttx_abstract_same(root, ttx_none()) ||
      !ttx_abstract_same(dialect, provider_candidate)) {
    interpreted.graph.operations->release(interpreted.graph.self);
    return invalid;
  }

  const auto validation = Language::validate(interpreted.graph);
  if (validation.state == Language::ValidationState::Invalid) {
    interpreted.graph.operations->release(interpreted.graph.self);
    return invalid;
  }

  Dynamic::Record<ProviderSource> retained(transaction, interpreted.graph);
  provider_sources.insert(retained);
  retained_graphs.insert(
      arena.proxy(semantic_name), {
                                    .root = root,
                                    .local = nullptr,
                                  });
  switch (validation.state) {
  case Language::ValidationState::Accepted:
    return {
      .state = ProviderSourceState::Accepted,
      .root = root,
      .error = {},
    };
  case Language::ValidationState::Incomplete:
    return {
      .state = ProviderSourceState::Incomplete,
      .root = root,
      .error = {},
    };
  case Language::ValidationState::Failed:
    return {
      .state = ProviderSourceState::Failed,
      .root = root,
      .error = validation.error,
    };
  case Language::ValidationState::Invalid:
    break;
  }
  return invalid;
}

auto Environment::Workspace::produce(
    ttx_abstract root,
    ttx_context context,
    Allocator::Arena& result_arena,
    tetrodotoxin_production_result result) const -> Bool {
  if (root.operations == nullptr || context.operations == nullptr ||
      result.operations == nullptr || result.self == nullptr ||
      result.operations->header.abi_major != TTX_ABI_MAJOR ||
      result.operations->header.size <
          sizeof(tetrodotoxin_production_result_ops)) {
    return False;
  }

  for (Count index = 0; index < provider_sources.get_size(); ++index) {
    const tetrodotoxin_source_graph graph = provider_sources[index]->graph;
    if (!ttx_abstract_same(graph.operations->root(graph.self), root)) {
      continue;
    }
    graph.operations->produce(graph.self, context, result);
    return True;
  }

  for (Count index = 0; index < retained_graphs.get_size(); ++index) {
    const auto* entry = retained_graphs.get_entry(index);
    if (entry == nullptr || entry->value.local == nullptr ||
        !ttx_abstract_same(entry->value.root, root)) {
      continue;
    }
    const Language::Monograph& monograph = *entry->value.local;
    auto dialect = monograph.get_language().select<Language::Dialect>();
    if (!dialect) {
      return False;
    }
    dialect->produce(
        context, result_arena, get_provider_handle(), monograph, result);
    return True;
  }
  return False;
}

auto Environment::Workspace::produce(
    ttx_abstract root,
    ttx_context context,
    Allocator::Arena& result_arena) const -> Language::ProductionObservation {
  Language::ProductionSink result;
  if (!produce(root, context, result_arena, result.get_handle())) {
    return {
      .state = Language::ProductionState::Invalid,
      .production = {},
      .error = {},
    };
  }
  return result.get_observation();
}

auto Environment::Workspace::import_package(
    Errors& errors,
    View::Bytes package_root,
    View::Bytes root_semantic_name,
    View::Bytes root_logical_route) -> Option<Language::Monograph&> {
  // Reading the Package root creates the first authored Cursor. Failures before
  // that point belong to Package acquisition, while later failures can use the
  // exact source text and Anchor from that Cursor.
  Allocator::Arena acquisition;

  if (!snapshots) {
    snapshots = Dynamic::Record<Package::Snapshots>();
  }

  auto storage = Package::Storage::open(acquisition, package_root, *snapshots);
  if (!storage) {
    Diagnostics::Log::Message<768> message(Diagnostics::Log::Level::Error);
    message
        << "Package import could not open its confined filesystem root `"_view
        << package_root << "`."_view;
    return {};
  }

  Option<Package::Content&> manifest =
      storage->read(root_logical_route)
          .visit(
              [](Package::Content& content) {
                return Option<Package::Content&>(content);
              },
              [&](const Package::Storage::Failure& failure) {
                Diagnostics::Log::Message<768> message(
                    Diagnostics::Log::Level::Error);
                append_storage_failure(
                    message, failure, "Package root source"_view);
                return Option<Package::Content&>();
              });
  if (!manifest) {
    return {};
  }

  // The Package root begins the candidate graph. Its bytes, Tokens, Cursor, and
  // Package Monograph share one Arena, so a rejected import releases every
  // borrowed view together.
  Dynamic::Record<Allocator::Arena> root_transaction;
  View::Bytes root_contents = root_transaction->proxy(manifest->get_contents());
  View::Bytes root_path =
      root_transaction->proxy(manifest->get_diagnostic_path());
  Tokenizer& root_tokenizer = root_transaction->construct<Tokenizer>(
      *root_transaction, root_contents, root_path);
  Associations& root_associations =
      root_transaction->construct<Associations>(*root_transaction);
  Cursor& root_cursor = root_transaction->construct<Cursor>(
      root_tokenizer, errors, root_associations, root_logical_route);
  if (retained_graphs.contains(root_semantic_name)) {
    root_cursor.create_error(
        "This Package semantic name is already published in the Workspace."_view,
        root_semantic_name);
    return {};
  }

  // The Package root contributes only its Library export surface and common
  // Import Types. Workspace discovers the complete source graph by walking
  // those external Type edges.
  Count root_error_count = errors.get_size();
  auto root_interpretation = Language::Dialect::interpret_source(
      toolchain.get_dialects(), root_cursor, *this);
  if (!root_interpretation) {
    if (errors.get_size() == root_error_count) {
      root_cursor.create_error(
          "Package root interpretation failed without a more specific "
          "diagnostic."_view);
    }
    return {};
  }

  auto selected_root =
      root_interpretation->select<Package::Language::Monograph>();
  if (!selected_root) {
    root_cursor.create_error(
        "The root source of a Package import must use the installed Package "
        "Dialect."_view);
    return {};
  }
  Package::Language::Monograph& root = *selected_root;
  View::Bytes root_package_identity = root.get_name();
  Version root_package_version = root.get_version();
  if (active_packages.get_size() != 0) {
    const ActivePackage& requested =
        active_packages[active_packages.get_size() - 1];
    if (requested.identity != root_package_identity ||
        requested.version != root_package_version) {
      root_cursor.create_error(
          "The Package source does not match the requested coordinate."_view,
          requested.identity);
      return {};
    }
  }
  for (Count i = 0; i < packages.get_size(); i++) {
    const ImportedPackage& imported = packages[i];
    if (imported.identity != root_package_identity) {
      continue;
    }

    root_cursor.create_error(
        imported.version == root_package_version
            ? "This exact Package identity and version is already imported "
              "into the Workspace."_view
            : "This Package identity is already imported with a different "
              "version."_view,
        root_package_identity);
    return {};
  }

  Dynamic::Vector<Dynamic::Record<Allocator::Arena>> candidate_transactions;
  Managed::Vector<Language::Monograph*> candidates(acquisition);
  Managed::Vector<Cursor*> cursors(acquisition);
  Managed::Vector<View::Bytes> diagnostic_paths(acquisition);
  Managed::Vector<View::Bytes> logical_routes(acquisition);
  Managed::Vector<View::Bytes> source_names(acquisition);
  Managed::Vector<Bool> parse_validity(acquisition);
  candidate_transactions.insert(root_transaction);
  candidates.insert(&root);
  cursors.insert(&root_cursor);
  diagnostic_paths.insert(root_path);
  logical_routes.insert(root_logical_route);
  source_names.insert(root_semantic_name);
  parse_validity.insert(errors.get_size() == root_error_count);

  Bool retained = False;
  auto retain_candidates = [&](Bool completed) {
    if (retained) {
      return;
    }

    View::Bytes retained_package_root = arena.proxy(package_root);
    for (Count index = 0; index < candidate_transactions.get_size(); index++) {
      retained_sources.insert({
        .package_root = retained_package_root,
        .name = source_names[index],
        .logical_route = logical_routes[index],
        .diagnostic_path = diagnostic_paths[index],
        .source_text = cursors[index]->get_source_text(),
        .transaction = candidate_transactions[index],
        .monograph = *candidates[index],
        .tokens = cursors[index]->get_tokens(),
        .associations = cursors[index]->get_associations(),
        .completed = completed,
      });
      if (index != 0) {
        package_members.insert({
          .package = &root,
          .name = arena.proxy(source_names[index]),
          .logical_route = arena.proxy(logical_routes[index]),
          .monograph = candidates[index],
        });
      }
    }
    retained_graphs.insert(
        arena.proxy(root_semantic_name), {
                                           .root = root.get_handle(),
                                           .local = &root,
                                         });
    retained = True;
  };

  // Package resources borrow this import's confined Storage while sources are
  // parsed. Sealing it before linking leaves later semantic stages with only
  // the resources the Package already selected.
  if (!root.get_resources().connect(*storage)) {
    root_cursor.create_error(
        "The Package resource table rejected its one import storage."_view);
    retain_candidates(False);
    return {};
  }

  Bool parsed = parse_validity[0];

  // Common Import Types own the new source graph. Each local locator is
  // resolved relative to the importing source, while package imports terminate
  // at one already restored exact Package product. Each Import retains that
  // acquisition while its ordinary Type expression selects the visible target.
  for (Count candidate_index = 0; candidate_index < candidates.get_size();
       candidate_index++) {
    Language::Monograph& importer = *candidates[candidate_index];
    for (Language::Import* retained_type : importer.get_imports()) {
      Language::Import& import = *retained_type;
      if (import.get_acquired()) {
        continue;
      }

      if (import.get_kind() == Language::Import::Kind::Package) {
        Option<const ImportedPackage&> selected;
        for (Count package_index = 0; package_index < packages.get_size();
             package_index++) {
          const ImportedPackage& candidate = packages[package_index];
          if (candidate.identity == import.get_locator() &&
              candidate.version == import.get_version()) {
            selected = candidate;
            break;
          }
        }
        if (!selected &&
            restore_coordinate(
                errors, import.get_locator(), import.get_version())) {
          for (const ImportedPackage& candidate : packages.get_view()) {
            if (candidate.identity == import.get_locator() &&
                candidate.version == import.get_version()) {
              selected = candidate;
              break;
            }
          }
        }
        auto target =
            selected
                ? selected->monograph->get_root().select<Ttx::Model::Domain>()
                : Option<const Ttx::Model::Domain&>();
        if (!target || !import.acquire(*target)) {
          cursors[candidate_index]->create_expression_error(
              import.get_declaration_anchor(),
              "Package Import did not select one restored exact Package."_view,
              import.get_locator());
          parse_validity[candidate_index] = False;
          parsed = False;
        }
        continue;
      }

      Path route(logical_routes[candidate_index], import.get_locator());
      View::Bytes normalized_route = route.get_view();
      if (normalized_route.is_empty() || route.is_rooted()) {
        cursors[candidate_index]->create_expression_error(
            import.get_declaration_anchor(),
            "Source Import did not resolve to one confined relative path."_view,
            import.get_locator());
        parse_validity[candidate_index] = False;
        parsed = False;
        continue;
      }

      Option<Count> existing_index;
      for (Count index = 0; index < logical_routes.get_size(); index++) {
        if (logical_routes[index] == normalized_route) {
          existing_index = index;
          break;
        }
      }

      if (!existing_index) {
        Option<Package::Content&> content =
            storage->read(normalized_route)
                .visit(
                    [](Package::Content& acquired) {
                      return Option<Package::Content&>(acquired);
                    },
                    [&](const Package::Storage::Failure& failure) {
                      auto report = cursors[candidate_index]->create_report(
                          import.get_declaration_anchor());
                      append_storage_failure(
                          report, failure, "Source Import"_view);
                      return Option<Package::Content&>();
                    });
        if (!content) {
          parse_validity[candidate_index] = False;
          parsed = False;
          continue;
        }

        Dynamic::Record<Allocator::Arena> source_transaction;
        View::Bytes source_contents =
            source_transaction->proxy(content->get_contents());
        View::Bytes source_path =
            source_transaction->proxy(content->get_diagnostic_path());
        View::Bytes retained_route =
            source_transaction->proxy(normalized_route);
        Tokenizer& tokenizer = source_transaction->construct<Tokenizer>(
            *source_transaction, source_contents, source_path);
        Associations& associations =
            source_transaction->construct<Associations>(*source_transaction);
        Cursor& cursor = source_transaction->construct<Cursor>(
            tokenizer, errors, associations, retained_route);
        Count source_error_count = errors.get_size();
        auto interpretation = Language::Dialect::interpret_source(
            toolchain.get_dialects(), cursor, root);
        if (!interpretation) {
          if (errors.get_size() == source_error_count) {
            cursor.create_error(
                "Imported source interpretation failed without a more specific diagnostic."_view);
          }
          parse_validity[candidate_index] = False;
          parsed = False;
          continue;
        }
        if (interpretation->is<Package::Language::Monograph>()) {
          cursor.create_error(
              "A local source Import cannot begin another Package product."_view,
              "Use package(.name = ..., .version = ...) at that boundary."_view);
          parse_validity[candidate_index] = False;
          parsed = False;
          continue;
        }

        candidate_transactions.insert(source_transaction);
        candidates.insert(&*interpretation);
        cursors.insert(&cursor);
        diagnostic_paths.insert(source_path);
        logical_routes.insert(retained_route);
        Managed::Bytes semantic_route(*source_transaction);
        if (candidate_index != 0) {
          semantic_route.concat(source_names[candidate_index]);
          semantic_route.concat("::"_view);
        }
        semantic_route.concat(import.get_name());
        source_names.insert(semantic_route.get_view());
        parse_validity.insert(errors.get_size() == source_error_count);
        existing_index = candidates.get_size() - 1;
      }

      auto target =
          candidates[*existing_index]->get_root().select<Ttx::Model::Domain>();
      if (!target || !import.acquire(*target)) {
        cursors[candidate_index]->create_expression_error(
            import.get_declaration_anchor(),
            "Source Import Type could not acquire its semantic root."_view,
            import.get_locator());
        parse_validity[candidate_index] = False;
        parsed = False;
      }
    }
  }

  // Parsing turns retained Package bytes into source-owned graph identities.
  // Linking receives the sealed context after that conversion, when the set of
  // semantic candidates is already fixed.
  root.get_resources().seal();

  // External source Types determine completion order. Parsing established every
  // identity already; this dependency-first order now lets each imported
  // source settle its generated and authored Types before an importer validates
  // those layouts. Package imports terminate at graphs restored earlier.
  Managed::Vector<U8> source_states(acquisition);
  Managed::Vector<Count> source_order(acquisition);
  for (Count index = 0; index < candidates.get_size(); index++) {
    source_states.insert(0);
  }
  auto order_source = [&](auto& self, Count index) -> Bool {
    if (source_states[index] == 2) {
      return True;
    }
    if (source_states[index] == 1) {
      cursors[index]->create_error(
          "Source Import graph contains a cycle."_view,
          "Break the cycle or place the shared Types in a third source."_view);
      return False;
    }
    source_states[index] = 1;
    for (Language::Import* retained_type : candidates[index]->get_imports()) {
      Language::Import& import = *retained_type;
      if (import.get_kind() != Language::Import::Kind::Source) {
        continue;
      }
      auto acquired = import.get_acquired();
      Option<Count> target_index;
      for (Count candidate_index = 0; candidate_index < candidates.get_size();
           candidate_index++) {
        if (acquired &&
            &candidates[candidate_index]->get_root() == &*acquired) {
          target_index = candidate_index;
          break;
        }
      }
      if (!target_index || !self(self, *target_index)) {
        return False;
      }
    }
    source_states[index] = 2;
    source_order.insert(index);
    return True;
  };
  for (Count index = 0; index < candidates.get_size(); index++) {
    if (!order_source(order_source, index)) {
      parsed = False;
    }
  }

  // Validation visits every retained candidate so tooling keeps
  // the strongest graph it can observe. Publication remains closed when any
  // earlier parse step failed.
  Bool linked = parsed;
  for (Count ordered = 0; ordered < source_order.get_size(); ordered++) {
    Count i = source_order[ordered];
    Count source_error_count = errors.get_size();
    Bool imports_resolved = True;
    for (Language::Import* retained_type : candidates[i]->get_imports()) {
      imports_resolved &= retained_type->validate(*cursors[i]);
    }
    Bool candidate_linked =
        imports_resolved && candidates[i]->link(*cursors[i]);
    if (!candidate_linked) {
      if (parse_validity[i] && errors.get_size() == source_error_count) {
        cursors[i]->create_error(
            "Package source linking failed without a more specific "
            "diagnostic."_view);
      }
      linked = False;
    }
  }

  // Finalization can consume linked declarations from any member. Waiting for
  // the whole graph to link gives each candidate the same completed context.
  Bool finalized = linked;
  if (linked) {
    for (Count ordered = 0; ordered < source_order.get_size(); ordered++) {
      Count i = source_order[ordered];
      Count source_error_count = errors.get_size();
      if (!candidates[i]->finalize(*cursors[i])) {
        if (errors.get_size() == source_error_count) {
          cursors[i]->create_error(
              "Package source finalization failed without a more specific "
              "diagnostic."_view);
        }
        finalized = False;
      }
    }
  }

  retain_candidates(finalized);
  if (!finalized) {
    return {};
  }

  View::Bytes retained_identity = arena.proxy(root_package_identity);
  packages.insert({
    .identity = retained_identity,
    .version = root_package_version,
    .monograph = &root,
  });
  return root;
}

auto Environment::Workspace::restore_package(
    const Package::Archive::Archive& archive,
    View::Bytes root_semantic_name) -> Option<Language::Monograph&> {
  if (root_semantic_name.is_empty() ||
      retained_graphs.contains(root_semantic_name)) {
    Diagnostics::Log::error(
        "Package restoration requires one unpublished semantic name."_view);
    return {};
  }

  for (Count index = 0; index < packages.get_size(); index++) {
    const ImportedPackage& imported = packages[index];
    if (imported.identity == archive.get_identity()) {
      Diagnostics::Log::error(
          "Package restoration cannot publish one identity twice."_view);
      return {};
    }
  }

  Dynamic::Record<Allocator::Arena> root_transaction;
  auto package_dialect = toolchain.find("Package"_view);
  if (!package_dialect || !package_dialect->is<Package::Dialect>()) {
    Diagnostics::Log::error(
        "Package restoration requires the installed Package Dialect."_view);
    return {};
  }
  Managed::Vector<Package::Resource*> resources(*root_transaction);
  for (const Package::Archive::Resource& archived : archive.get_resources()) {
    resources.insert(&Package::Resource::create(
        *root_transaction, archived.get_route(), archived.get_value()));
  }
  Package::Language::Monograph& root =
      Package::Language::Monograph::create_synthetic(
          *root_transaction, *package_dialect, archive.get_identity(),
          archive.get_version(), *this,
          static_cast<Package::Dialect&>(*package_dialect).get_library(),
          resources.get_view());

  Dynamic::Vector<Dynamic::Record<Allocator::Arena>> candidates;
  Managed::Vector<Language::Monograph*> monographs(*root_transaction);
  Managed::Vector<View::Bytes> restored_member_names(*root_transaction);
  Managed::Map<View::Bytes, Language::Monograph&> restored_members(
      *root_transaction);
  candidates.insert(root_transaction);
  monographs.insert(&root);
  for (const Package::Archive::Member& member : archive.get_members()) {
    auto dialect = toolchain.find(member.get_dialect_name());
    if (!dialect) {
      Diagnostics::Log::error(
          "Package restoration requires every member Dialect installed."_view);
      return {};
    }

    if (member.get_semantic_name() == "PackageSurface"_view) {
      if (&*dialect !=
              &static_cast<Package::Dialect&>(*package_dialect).get_library() ||
          !Library::Archive::Reader::restore_source(
              *root_transaction, member.get_payload(),
              root.edit_library().get_source())) {
        Diagnostics::Log::error(
            "Package restoration rejected its Library export surface."_view);
        return {};
      }
      continue;
    }

    Dynamic::Record<Allocator::Arena> transaction;
    auto restored = dialect->restore(
        *transaction, member.get_payload(), Documentation::get_empty(), root);
    View::Bytes member_name =
        root_transaction->proxy(member.get_semantic_name());
    if (!restored || restored->is<Package::Language::Monograph>()) {
      Diagnostics::Log::Message<256> message(
          Diagnostics::Log::Level::Error, Diagnostics::Source());
      message << "Package restoration rejected member `"_view << member_name
              << "` for the "_view << member.get_dialect_name()
              << " Dialect."_view;
      return {};
    }

    candidates.insert(transaction);
    monographs.insert(&*restored);
    restored_members.launder(member_name, *restored);
    restored_member_names.insert(member_name);
  }

  for (const Package::Archive::GraphImport& archived : archive.get_imports()) {
    Language::Monograph* importer = nullptr;
    if (archived.get_importer() == "PackageSurface"_view) {
      importer = &root;
    } else {
      auto selected = restored_members.find(archived.get_importer());
      if (selected) {
        importer = &selected->value;
      }
    }
    if (!importer) {
      Diagnostics::Log::error(
          "Package restoration could not select one Import owner."_view);
      return {};
    }

    Language::Import::Description description(
        root_transaction->proxy(archived.get_local_name()),
        Documentation::get_empty(), archived.get_visibility(),
        archived.get_kind(), root_transaction->proxy(archived.get_target()),
        archived.get_version(), root_transaction->proxy(archived.get_route()),
        Anchor::create(Span()), Anchor::create(Span()), Anchor::create(Span()));
    if (!importer->retain_import(description)) {
      Diagnostics::Log::error(
          "Package restoration could not retain one Import Type."_view);
      return {};
    }
    Language::Import& import =
        *importer->get_imports()
             .get_data()[importer->get_imports().get_size() - 1];

    const Ttx::Model::Domain* target = nullptr;
    if (archived.get_kind() == Language::Import::Kind::Source) {
      auto selected = restored_members.find(archived.get_target());
      if (selected) {
        auto root_type =
            selected->value.get_root().select<Ttx::Model::Domain>();
        if (root_type) {
          target = &*root_type;
        }
      }
    } else {
      for (const ImportedPackage& candidate : packages.get_view()) {
        if (candidate.identity == archived.get_target() &&
            candidate.version == archived.get_version()) {
          auto root_type =
              candidate.monograph->get_root().select<Ttx::Model::Domain>();
          if (root_type) {
            target = &*root_type;
          }
          break;
        }
      }
    }
    if (!target || !import.acquire(*target)) {
      Diagnostics::Log::error(
          "Package restoration could not acquire one external Type."_view);
      return {};
    }
  }

  Managed::Vector<U8> restored_states(*root_transaction);
  Managed::Vector<Count> restored_order(*root_transaction);
  for (Count index = 0; index < monographs.get_size(); index++) {
    restored_states.insert(0);
  }
  auto order_restored = [&](auto& self, Count index) -> Bool {
    if (restored_states[index] == 2) {
      return True;
    }
    BAIL_IF(restored_states[index] == 1);
    restored_states[index] = 1;
    for (Language::Import* retained_type : monographs[index]->get_imports()) {
      Language::Import& import = *retained_type;
      if (import.get_kind() != Language::Import::Kind::Source) {
        continue;
      }
      auto acquired = import.get_acquired();
      Option<Count> target_index;
      for (Count candidate = 0; candidate < monographs.get_size();
           candidate++) {
        if (acquired && &monographs[candidate]->get_root() == &*acquired) {
          target_index = candidate;
          break;
        }
      }
      BAIL_IF(!target_index || !self(self, *target_index));
    }
    restored_states[index] = 2;
    restored_order.insert(index);
    return True;
  };
  for (Count index = 0; index < monographs.get_size(); index++) {
    if (!order_restored(order_restored, index)) {
      Diagnostics::Log::error(
          "Package restoration rejected a cyclic Source Import graph."_view);
      return {};
    }
  }

  for (Count ordered = 0; ordered < restored_order.get_size(); ordered++) {
    Count index = restored_order[ordered];
    for (Language::Import* retained_type : monographs[index]->get_imports()) {
      Language::Import& import = *retained_type;
      if (!import.validate_restored()) {
        Diagnostics::Log::Message<256> message(
            Diagnostics::Log::Level::Error, Diagnostics::Source());
        message << "Package restoration could not resolve external Type `"_view
                << import.get_name() << "` from `"_view << import.get_locator()
                << "`"_view;
        if (!import.get_route().is_empty()) {
          message << "::"_view << import.get_route();
        }
        message << "."_view;
        return {};
      }
    }

    if (!monographs[index]->link_restored()) {
      Diagnostics::Log::error(
          "Package restoration failed while linking member graphs."_view);
      return {};
    }
  }
  for (Count ordered = 0; ordered < restored_order.get_size(); ordered++) {
    Count index = restored_order[ordered];
    if (!monographs[index]->finalize_restored()) {
      Diagnostics::Log::error(
          "Package restoration failed while finalizing member graphs."_view);
      return {};
    }
  }

  for (const Dynamic::Record<Allocator::Arena>& candidate :
       candidates.get_view()) {
    restored_transactions.insert(candidate);
  }
  for (Count index = 0; index < restored_member_names.get_size(); index++) {
    View::Bytes name = arena.proxy(restored_member_names[index]);
    package_members.insert({
      .package = &root,
      .name = name,
      .logical_route = name,
      .monograph = monographs[index + 1],
    });
  }
  View::Bytes retained_identity = arena.proxy(archive.get_identity());
  packages.insert({
    .identity = retained_identity,
    .version = archive.get_version(),
    .monograph = &root,
  });
  View::Bytes retained_name = arena.proxy(root_semantic_name);
  retained_graphs.insert(
      retained_name, {
                       .root = root.get_handle(),
                       .local = &root,
                     });
  return root;
}

auto Environment::Workspace::get_associations(View::Bytes diagnostic_path) const
    -> Option<const Associations&> {
  for (Count i = 0; i < retained_sources.get_size(); i++) {
    const RetainedSource& source = retained_sources[i];
    if (source.diagnostic_path == diagnostic_path) {
      return source.associations;
    }
  }

  return {};
}

auto Environment::Workspace::get_associations(
    View::Bytes package_root,
    View::Bytes logical_route) const -> Option<const Associations&> {
  const RetainedSource* source =
      find_retained_source(package_root, logical_route);
  return source ? Option<const Associations&>(source->associations)
                : Option<const Associations&>();
}

auto Environment::Workspace::get_monograph(View::Bytes diagnostic_path) const
    -> Option<const Language::Monograph&> {
  for (const RetainedSource& source : retained_sources.get_view()) {
    if (source.diagnostic_path == diagnostic_path) {
      return source.monograph;
    }
  }
  return {};
}

auto Environment::Workspace::get_monograph(
    View::Bytes package_root,
    View::Bytes logical_route) const -> Option<const Language::Monograph&> {
  const RetainedSource* source =
      find_retained_source(package_root, logical_route);
  return source ? Option<const Language::Monograph&>(source->monograph)
                : Option<const Language::Monograph&>();
}

auto Environment::Workspace::get_completed_monograph(
    View::Bytes diagnostic_path) const -> Option<const Language::Monograph&> {
  for (const RetainedSource& source : retained_sources.get_view()) {
    if (source.diagnostic_path == diagnostic_path && source.completed) {
      return source.monograph;
    }
  }
  return {};
}

auto Environment::Workspace::get_completed_monograph(
    View::Bytes package_root,
    View::Bytes logical_route) const -> Option<const Language::Monograph&> {
  const RetainedSource* source =
      find_retained_source(package_root, logical_route);
  return source && source->completed
             ? Option<const Language::Monograph&>(source->monograph)
             : Option<const Language::Monograph&>();
}

auto Environment::Workspace::get_package_source_count(
    const Package::Language::Monograph& package) const -> Count {
  Count count = 0;
  for (const PackageMember& member : package_members.get_view()) {
    if (member.package == &package) {
      count++;
    }
  }
  return count;
}

auto Environment::Workspace::get_package_source(
    const Package::Language::Monograph& package,
    Count selected_index) const -> Option<PackageSource> {
  Count index = 0;
  for (const PackageMember& member : package_members.get_view()) {
    if (member.package != &package) {
      continue;
    }
    if (index++ == selected_index) {
      return PackageSource(
          member.name, member.logical_route, *member.monograph);
    }
  }
  return {};
}

auto Environment::Workspace::find_package_owner(const Abstract& semantic) const
    -> Option<const Package::Language::Monograph&> {
  const Abstract& selected = semantic.resolve();
  for (const PackageMember& member : package_members.get_view()) {
    if (&member.monograph->get_root().resolve() == &selected) {
      return *member.package;
    }
  }
  for (const ImportedPackage& imported : packages.get_view()) {
    if (&imported.monograph->resolve() == &selected) {
      return *imported.monograph;
    }
  }
  return {};
}

auto Environment::Workspace::find_source_input(
    const Language::Monograph& monograph) const -> Option<AuthoredLocation> {
  for (const RetainedSource& source : retained_sources.get_view()) {
    if (&source.monograph == &monograph) {
      return AuthoredLocation(
          source.package_root, source.diagnostic_path, source.source_text,
          Anchor::create(Span()));
    }
  }
  return {};
}

auto Environment::Workspace::get_tokens(View::Bytes diagnostic_path) const
    -> View::Vector<Token> {
  for (const RetainedSource& source : retained_sources.get_view()) {
    if (source.diagnostic_path == diagnostic_path) {
      return source.tokens;
    }
  }
  return {};
}

auto Environment::Workspace::get_tokens(
    View::Bytes package_root,
    View::Bytes logical_route) const -> View::Vector<Token> {
  const RetainedSource* source =
      find_retained_source(package_root, logical_route);
  return source ? source->tokens : View::Vector<Token>();
}

auto Environment::Workspace::find_retained_source(
    View::Bytes package_root,
    View::Bytes logical_route) const -> const RetainedSource* {
  for (Count index = 0; index < retained_sources.get_size(); index++) {
    const RetainedSource& source = retained_sources[index];
    if (source.package_root == package_root &&
        source.logical_route == logical_route) {
      return &source;
    }
  }

  return nullptr;
}

auto Environment::Workspace::get_associations(
    const Language::Monograph& monograph) const -> Option<const Associations&> {
  for (Count i = 0; i < retained_sources.get_size(); i++) {
    const RetainedSource& source = retained_sources[i];
    if (&source.monograph == &monograph) {
      return source.associations;
    }
  }

  return {};
}

auto Environment::Workspace::find_authored_location(
    const Abstract& semantic) const -> Option<AuthoredLocation> {
  Option<Anchor> declaration;
  semantic.visit<Library::Language::Model::Type>(
      [&](const Library::Language::Model::Type& type) {
        declaration = type.get_declaration_anchor();
      },
      [&](const Abstract& candidate) {
        candidate.visit<Library::Language::Model::Addressable>(
            [&](const Library::Language::Model::Addressable& addressable) {
              declaration = addressable.get_declaration_anchor();
            },
            [&](const Abstract& callable_candidate) {
              callable_candidate.visit<Library::Language::Model::Callable>(
                  [&](const Library::Language::Model::Callable& callable) {
                    declaration = callable.get_declaration_anchor();
                  },
                  [&](const Abstract& import_candidate) {
                    auto import = import_candidate.select<Language::Import>();
                    if (import) {
                      declaration = import->get_declaration_anchor();
                    }
                  });
            });
      });

  if (declaration) {
    Token focus = declaration->get_token();
    Span span = declaration->get_span();
    for (const RetainedSource& source : retained_sources.get_view()) {
      for (const Associations::Entry& entry :
           source.associations.get_entries()) {
        if (&entry.get_semantic() != &semantic) {
          continue;
        }
        Anchor candidate = entry.get_anchor();
        Token candidate_focus = candidate.get_token();
        Span candidate_span = candidate.get_span();
        Bool same_focus =
            bool(focus) == bool(candidate_focus) &&
            (!focus || (focus.get_offset() == candidate_focus.get_offset() &&
                        focus.get_size() == candidate_focus.get_size() &&
                        focus.get_code() == candidate_focus.get_code()));
        Bool same_span =
            bool(span) == bool(candidate_span) &&
            (!span || (span.get_offset() == candidate_span.get_offset() &&
                       span.get_size() == candidate_span.get_size()));
        if ((focus && same_focus) || (!focus && same_span)) {
          return AuthoredLocation(
              source.package_root, source.diagnostic_path, source.source_text,
              candidate);
        }
      }
    }
  }

  for (const RetainedSource& source : retained_sources.get_view()) {
    auto anchor = source.associations.find(semantic);
    if (anchor) {
      return AuthoredLocation(
          source.package_root, source.diagnostic_path, source.source_text,
          *anchor);
    }
  }

  return {};
}

auto Environment::Workspace::find_acquired_location(
    View::Bytes package_root,
    View::Bytes logical_route,
    Count offset,
    const Abstract& semantic) const -> Option<AuthoredLocation> {
  const RetainedSource* importer =
      find_retained_source(package_root, logical_route);
  BAIL_IF(importer == nullptr);

  Token selected;
  for (const Token& token : importer->tokens) {
    Count start = token.get_offset();
    Count end = start + token.get_size();
    if (offset >= start && offset < end) {
      selected = token;
      break;
    }
  }
  BAIL_IF(!selected);

  Code::Type code = selected.get_code().get_type();
  if (code == Code::Type::Source || code == Code::Type::Package) {
    auto import = semantic.select<Language::Import>();
    BAIL_IF(
        !import ||
        (code == Code::Type::Source &&
         import->get_kind() != Language::Import::Kind::Source) ||
        (code == Code::Type::Package &&
         import->get_kind() != Language::Import::Kind::Package));

    auto target = import->get_acquired();
    BAIL_IF(!target);
    for (const RetainedSource& source : retained_sources.get_view()) {
      if (&source.monograph.get_root() == &*target) {
        return AuthoredLocation(
            source.package_root, source.diagnostic_path, source.source_text,
            Anchor::create(Span()));
      }
    }

    return {};
  }

  BAIL_IF(code != Code::Type::Embedded);
  auto resource = semantic.select<Package::Resource>();
  BAIL_IF(!resource);

  auto package = importer->monograph.select<Package::Language::Monograph>();
  const Package::Language::Monograph* owner = package ? &*package : nullptr;
  if (owner == nullptr) {
    for (const PackageMember& member : package_members.get_view()) {
      if (member.monograph == &importer->monograph) {
        owner = member.package;
        break;
      }
    }
  }
  BAIL_IF(owner == nullptr);

  Bool retained = owner->get_resources().get_values().contains(
      [&](Package::Resource* candidate) { return candidate == &*resource; });
  BAIL_IF(!retained);
  return AuthoredLocation(
      importer->package_root, resource->get_route(), {},
      Anchor::create(Span()));
}

auto Environment::Workspace::get_name() const -> View::Bytes {
  return "Workspace"_view;
}

auto Environment::Workspace::negotiate(ttx_abstract requirement) const
    -> ttx_interface_relation {
  return ttx_abstract_same(requirement, tetrodotoxin_workspace_requirement())
             ? TTX_INTERFACE_SATISFIED
             : Abstract::negotiate(requirement);
}

auto Environment::Workspace::get_documentation() const -> const Documentation& {
  return Documentation::get_empty();
}

auto Environment::Workspace::resolve() const -> const Abstract& {
  return *this;
}

auto Environment::Workspace::resolve_concept(View::Bytes route) const
    -> const Abstract& {
  return retained_graphs.visit(
      route,
      [](const RetainedGraph& selected) -> const Abstract& {
        return selected.local == nullptr
                   ? static_cast<const Abstract&>(Unknown::get_unknown())
                   : static_cast<const Abstract&>(*selected.local);
      },
      [&]() -> const Abstract& {
        return outer == nullptr
                   ? static_cast<const Abstract&>(Unknown::get_unknown())
                   : outer->resolve_concept(route);
      });
}

auto Environment::Workspace::visit_concepts(
    ttx_named_abstract_callable* visitor) const -> void {
  for (Count index = 0; index < retained_graphs.get_size(); index++) {
    const auto* entry = retained_graphs.get_entry(index);
    if (entry != nullptr && entry->value.local != nullptr) {
      visit_concept(visitor, entry->key, *entry->value.local);
    }
  }
  for (const PackageMember& member : package_members.get_view()) {
    visit_concept(visitor, member.name, *member.monograph);
  }
  if (outer != nullptr) {
    outer->visit_concepts(visitor);
  }
}

auto Environment::Workspace::resolve_concept(ttx_borrowed_bytes route) const
    -> ttx_abstract {
  const View::Bytes name(route.data, route.size);
  return retained_graphs.visit(
      name,
      [](const RetainedGraph& selected) -> ttx_abstract {
        return selected.root;
      },
      [&]() -> ttx_abstract {
        return outer == nullptr
                   ? ttx_unknown()
                   : Ttx::resolve_concept(outer->get_handle(), route);
      });
}

struct ForwardedConcepts {
  ttx_concept_sink_ops operations;
  ttx_concept_sink destination;
  Bool completed;
};

static auto forwarded_concepts(ttx_concept_sink self) -> ForwardedConcepts& {
  return *reinterpret_cast<ForwardedConcepts*>(self.self);
}

static void TTX_CALL forward_concept(
    ttx_concept_sink self,
    ttx_borrowed_bytes route,
    ttx_abstract answer) {
  auto& forwarding = forwarded_concepts(self);
  forwarding.destination.operations->item(
      forwarding.destination, route, answer);
}

static void TTX_CALL complete_forwarding(ttx_concept_sink self) {
  forwarded_concepts(self).completed = True;
}

void Environment::Workspace::visit_concepts(ttx_concept_sink result) const {
  for (Count index = 0; index < retained_graphs.get_size(); index++) {
    const auto* entry = retained_graphs.get_entry(index);
    if (entry != nullptr) {
      result.operations->item(
          result,
          {
            .data = entry->key.get_data(),
            .size = entry->key.get_size(),
          },
          entry->value.root);
    }
  }
  for (const PackageMember& member : package_members.get_view()) {
    result.operations->item(
        result,
        {
          .data = member.name.get_data(),
          .size = member.name.get_size(),
        },
        member.monograph->get_handle());
  }
  if (outer != nullptr) {
    ForwardedConcepts forwarding = {
      .operations =
          {
            .header =
                {
                  .size = sizeof(ttx_concept_sink_ops),
                  .abi_major = TTX_ABI_MAJOR,
                  .abi_minor = TTX_ABI_MINOR,
                },
            .item = forward_concept,
            .completed = complete_forwarding,
          },
      .destination = result,
      .completed = False,
    };
    const ttx_concept_sink sink = {
      .operations = &forwarding.operations,
      .self = reinterpret_cast<ttx_concept_sink_self*>(&forwarding),
    };
    const ttx_abstract outer_handle = outer->get_handle();
    outer_handle.operations->visit_concepts(outer_handle, sink);
    if (!forwarding.completed) {
      result.operations->completed(result);
      return;
    }
  }
  result.operations->completed(result);
}
