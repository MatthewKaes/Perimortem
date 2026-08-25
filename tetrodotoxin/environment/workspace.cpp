// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/environment/workspace.hpp"

#include "perimortem/core/diagnostics/log.hpp"

#include "perimortem/memory/dynamic/record.hpp"

#include "perimortem/system/path.hpp"

#include "tetrodotoxin/package/content.hpp"
#include "tetrodotoxin/package/dialect.hpp"
#include "tetrodotoxin/package/language/parser/name.hpp"
#include "tetrodotoxin/package/resource.hpp"
#include "tetrodotoxin/package/storage.hpp"
#include "ttx/concept/invalid.hpp"
#include "ttx/lexical/cursor.hpp"
#include "ttx/lexical/tokenizer.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Perimortem::System;
using namespace Ttx::Concept;
using namespace Ttx::Lexical;
using namespace Tetrodotoxin;

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

Environment::Workspace::Workspace(
    Toolchain& selected_toolchain,
    Option<Dynamic::Record<Package::Snapshots>> selected_snapshots)
    : toolchain(selected_toolchain),
      snapshots(selected_snapshots),
      arena(),
      retained_sources(),
      retained_monographs(),
      packages(arena) {}

Environment::Workspace::~Workspace() = default;

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

  if (retained_monographs.contains(semantic_name)) {
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
    // A manifest needs the Package path because its member table forms one
    // completion barrier. Sending it through the direct path could expose an
    // Alias before the member that owns its target exists.
    if (completed) {
      cursor.create_error(
          "A Package manifest must be completed through Workspace Package "
          "import."_view);
    }
    return {};
  }

  // The Monograph proves that the Dialect established a durable source owner.
  // Retaining its transaction here preserves Tokens, Associations, and every
  // partial identity while later barriers decide product eligibility.
  Count retained_index = retained_sources.get_size();
  retained_sources.insert({
    .package_root = {},
    .diagnostic_path = retained_path,
    .source_text = retained_contents,
    .transaction = transaction,
    .monograph = monograph,
    .tokens = tokenizer.get_tokens(),
    .associations = associations,
    .completed = False,
  });

  // Linking can still enrich a retained graph after interpretation reports an
  // incomplete source form. The earlier report keeps publication closed while
  // editor queries gain any Types and declaration edges that did settle.
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

  View::Bytes retained_name = arena.proxy(semantic_name);
  retained_monographs.insert(
      retained_name, Ttx::Concept::Reference<Language::Monograph>(monograph));
  retained_sources[retained_index].completed = completed;
  return completed ? Option<Language::Monograph&>(monograph)
                   : Option<Language::Monograph&>();
}

auto Environment::Workspace::import_package(
    Errors& errors,
    View::Bytes package_root,
    View::Bytes root_semantic_name,
    View::Bytes root_logical_route,
    View::Bytes root_package_identity,
    Version root_package_version) -> Option<Language::Monograph&> {
  // Reading the manifest creates the first authored Cursor. Failures before
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
                    message, failure, "Package manifest"_view);
                return Option<Package::Content&>();
              });
  if (!manifest) {
    return {};
  }

  // The manifest begins the candidate graph. Its bytes, Tokens, Cursor, and
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
      root_tokenizer, errors, root_associations);
  if (retained_monographs.contains(root_semantic_name)) {
    root_cursor.create_error(
        "This Package semantic name is already published in the Workspace."_view,
        root_semantic_name);
    return {};
  }

  if (root_package_identity.is_empty() || root_package_version.is_null()) {
    root_cursor.create_error(
        "Package import requires a nonempty identity and non-null version."_view);
    return {};
  }

  for (Count i = 0; i < packages.get_size(); i++) {
    const ImportedPackage& imported = packages[i];
    if (imported.identity != root_package_identity) {
      continue;
    }

    root_cursor.create_error(
        imported.version == root_package_version
            ? "This exact Package identity and version is already imported "
              "into "
              "the Workspace."_view
            : "This Package identity is already imported with a different "
              "version."_view,
        root_package_identity);
    return {};
  }

  // Interpreting the manifest gives Workspace the complete Dependency and
  // Source table before it starts acquiring members.
  Count root_error_count = errors.get_size();
  auto root_interpretation = Language::Dialect::interpret_source(
      toolchain.get_dialects(), root_cursor, *this);
  if (!root_interpretation) {
    if (errors.get_size() == root_error_count) {
      root_cursor.create_error(
          "Package manifest interpretation failed without a more specific "
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

  Dynamic::Vector<Dynamic::Record<Allocator::Arena>> candidate_transactions(
      root.get_sources().get_size() + 1);
  Managed::Vector<Language::Monograph*> candidates(acquisition);
  Managed::Vector<Cursor*> cursors(acquisition);
  Managed::Vector<View::Bytes> diagnostic_paths(acquisition);
  Managed::Vector<Bool> parse_validity(acquisition);
  candidate_transactions.insert(root_transaction);
  candidates.insert(&root);
  cursors.insert(&root_cursor);
  diagnostic_paths.insert(root_path);
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
        .diagnostic_path = diagnostic_paths[index],
        .source_text = cursors[index]->get_source_text(),
        .transaction = candidate_transactions[index],
        .monograph = *candidates[index],
        .tokens = cursors[index]->get_tokens(),
        .associations = cursors[index]->get_associations(),
        .completed = completed,
      });
    }
    retained_monographs.insert(
        arena.proxy(root_semantic_name),
        Ttx::Concept::Reference<Language::Monograph>(root));
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

  // Dependencies arrive as completed Workspace facts. Matching their exact
  // identity and version keeps this import focused on the graph named by its
  // manifest.
  Bool parsed = parse_validity[0];
  View::Vector<Package::Language::Dependency> dependencies =
      root.get_dependencies();
  for (Count dependency_index = 0; dependency_index < dependencies.get_size();
       dependency_index++) {
    const Package::Language::Dependency& dependency =
        dependencies.get_data()[dependency_index];
    Option<const ImportedPackage&> selected;
    for (Count i = 0; i < packages.get_size(); i++) {
      const ImportedPackage& imported = packages[i];
      if (imported.identity == dependency.get_package_name()) {
        selected = imported;
        break;
      }
    }

    if (!selected) {
      root_cursor.create_expression_error(
          dependency.get_span(),
          "Package dependency is not already imported in this Workspace."_view,
          dependency.get_package_name());
      parsed = False;
      continue;
    }

    if (selected->version != dependency.get_version()) {
      root_cursor.create_expression_error(
          dependency.get_span(),
          "Package dependency requests a different version than the one "
          "already imported in this Workspace."_view,
          dependency.get_package_name());
      parsed = False;
      continue;
    }

    if (!root.bind_dependency(dependency, *selected->monograph)) {
      root_cursor.create_expression_error(
          dependency.get_span(),
          "Package dependency could not enter the Package mapping table."_view,
          dependency.get_local_name());
      parsed = False;
    }
  }

  // Each declared Source gets its own owner and Cursor. Source values and
  // diagnostics can then retain the exact text and location of that member.
  for (const Package::Language::Source& source : root.get_sources()) {
    Option<Package::Content&> content =
        storage->read(source.get_source_path())
            .visit(
                [&](Package::Content& acquired) {
                  return Option<Package::Content&>(acquired);
                },
                [&](const Package::Storage::Failure& failure) {
                  auto report = root_cursor.create_report(source.get_span());
                  append_storage_failure(
                      report, failure, "Package source"_view);
                  return Option<Package::Content&>();
                });
    if (!content) {
      parsed = False;
      continue;
    }

    Dynamic::Record<Allocator::Arena> source_transaction;
    View::Bytes source_contents =
        source_transaction->proxy(content->get_contents());
    View::Bytes source_path =
        source_transaction->proxy(content->get_diagnostic_path());
    Tokenizer& tokenizer = source_transaction->construct<Tokenizer>(
        *source_transaction, source_contents, source_path);
    Associations& associations =
        source_transaction->construct<Associations>(*source_transaction);
    Cursor& cursor =
        source_transaction->construct<Cursor>(tokenizer, errors, associations);
    Count source_error_count = errors.get_size();
    auto member_interpretation = Language::Dialect::interpret_source(
        toolchain.get_dialects(), cursor, root);
    if (!member_interpretation) {
      if (errors.get_size() == source_error_count) {
        cursor.create_error(
            "Package source interpretation failed without a more specific "
            "diagnostic."_view);
      }
      parsed = False;
      continue;
    }
    Language::Monograph& member = *member_interpretation;

    candidate_transactions.insert(source_transaction);
    candidates.insert(&member);
    cursors.insert(&cursor);
    diagnostic_paths.insert(source_path);
    parse_validity.insert(errors.get_size() == source_error_count);
    Count candidate_index = candidates.get_size() - 1;

    // The root manifest has already fixed the complete member table. Treating
    // one member as another Package would grow the table after its barrier and
    // leave the import order responsible for its shape.
    if (member.is<Package::Language::Monograph>()) {
      cursor.create_error(
          "A Package Source cannot create another Package import."_view);
      parse_validity[candidate_index] = False;
      parsed = False;
      continue;
    }

    if (!root.bind_member(source.get_local_route(), member)) {
      root_cursor.create_expression_error(
          source.get_span(),
          "Package source could not enter the Package mapping table."_view,
          source.get_local_name());
      parse_validity[candidate_index] = False;
      parsed = False;
      continue;
    }
    parsed &= parse_validity[candidate_index];
  }
  // Source parsing is where Package Storage becomes authored language facts.
  // Linking receives the sealed context after that conversion, when the set of
  // semantic candidates is already fixed.
  root.get_resources().seal();

  // Every parsed identity enters the candidate set before any member resolves
  // context. Authored Source order therefore cannot decide which routes are
  // visible. Each retained candidate gets the same chance to settle useful
  // graph edges, while any parse or link failure keeps the island unpublished.
  Bool linked = parsed;
  for (Count i = 0; i < candidates.get_size(); i++) {
    Count source_error_count = errors.get_size();
    Bool candidate_linked = candidates[i]->link(*cursors[i]);
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
    for (Count i = 0; i < candidates.get_size(); i++) {
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
      retained_monographs.contains(root_semantic_name)) {
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
  Managed::Vector<Package::Language::Dependency> dependencies(
      *root_transaction);
  for (const Package::Language::Dependency& dependency :
       archive.get_dependencies()) {
    dependencies.insert(
        Package::Language::Dependency(
            Package::Language::Parser::Name(
                root_transaction->proxy(dependency.get_local_name())),
            root_transaction->proxy(dependency.get_package_name()),
            dependency.get_version()));
  }
  auto package_dialect = toolchain.find("Package"_view);
  if (!package_dialect || !package_dialect->is<Package::Dialect>()) {
    Diagnostics::Log::error(
        "Package restoration requires the installed Package Dialect."_view);
    return {};
  }
  Managed::Vector<Reference<Package::Resource>> resources(*root_transaction);
  for (const Package::Archive::Resource& archived : archive.get_resources()) {
    resources.insert(
        Package::Resource::create(
            *root_transaction, archived.get_route(), archived.get_value()));
  }
  Package::Language::Monograph& root =
      Package::Language::Monograph::create_synthetic(
          *root_transaction, *package_dialect, *this, dependencies.get_view(),
          resources.get_view());

  View::Vector<Package::Language::Dependency> restored_dependencies =
      root.get_dependencies();
  for (Count dependency_index = 0;
       dependency_index < restored_dependencies.get_size();
       dependency_index++) {
    const Package::Language::Dependency& dependency =
        restored_dependencies.get_data()[dependency_index];
    Option<const ImportedPackage&> selected;
    for (Count index = 0; index < packages.get_size(); index++) {
      const ImportedPackage& imported = packages[index];
      if (imported.identity == dependency.get_package_name() &&
          imported.version == dependency.get_version()) {
        selected = imported;
        break;
      }
    }
    if (!selected || !root.bind_dependency(dependency, *selected->monograph)) {
      Diagnostics::Log::error(
          "Package restoration could not bind one exact dependency."_view);
      return {};
    }
  }

  Dynamic::Vector<Dynamic::Record<Allocator::Arena>> candidates;
  Managed::Vector<Language::Monograph*> monographs(*root_transaction);
  candidates.insert(root_transaction);
  monographs.insert(&root);
  for (const Package::Archive::Member& member : archive.get_members()) {
    auto dialect = toolchain.find(member.get_dialect_name());
    if (!dialect) {
      Diagnostics::Log::error(
          "Package restoration requires every member Dialect installed."_view);
      return {};
    }

    Dynamic::Record<Allocator::Arena> transaction;
    auto restored = dialect->restore(
        *transaction, member.get_payload(), archive.get_profile(),
        Documentation::get_empty(), root);
    View::Bytes member_name =
        root_transaction->proxy(member.get_semantic_name());
    if (!restored || restored->is<Package::Language::Monograph>() ||
        !root.bind_member(
            Package::Language::Parser::Name(member_name), *restored)) {
      Diagnostics::Log::Message<256> message(
          Diagnostics::Log::Level::Error, Diagnostics::Source());
      message << "Package restoration rejected member `"_view << member_name
              << "` for the "_view << member.get_dialect_name()
              << " Dialect."_view;
      return {};
    }

    candidates.insert(transaction);
    monographs.insert(&*restored);
  }

  for (Count index = 0; index < monographs.get_size(); index++) {
    if (!monographs[index]->link_restored()) {
      Diagnostics::Log::error(
          "Package restoration failed while linking member graphs."_view);
      return {};
    }
  }
  for (Count index = 0; index < monographs.get_size(); index++) {
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
  View::Bytes retained_identity = arena.proxy(archive.get_identity());
  packages.insert({
    .identity = retained_identity,
    .version = archive.get_version(),
    .monograph = &root,
  });
  View::Bytes retained_name = arena.proxy(root_semantic_name);
  retained_monographs.insert(
      retained_name, Ttx::Concept::Reference<Language::Monograph>(root));
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

auto Environment::Workspace::get_monograph(View::Bytes diagnostic_path) const
    -> Option<const Language::Monograph&> {
  for (const RetainedSource& source : retained_sources.get_view()) {
    if (source.diagnostic_path == diagnostic_path) {
      return source.monograph;
    }
  }
  return {};
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

auto Environment::Workspace::get_tokens(View::Bytes diagnostic_path) const
    -> View::Vector<Token> {
  for (const RetainedSource& source : retained_sources.get_view()) {
    if (source.diagnostic_path == diagnostic_path) {
      return source.tokens;
    }
  }
  return {};
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

auto Environment::Workspace::get_name() const -> View::Bytes {
  return "Workspace"_view;
}

auto Environment::Workspace::get_documentation() const -> const Documentation& {
  return Documentation::get_empty();
}

auto Environment::Workspace::resolve() const -> const Abstract& {
  return *this;
}

auto Environment::Workspace::resolve_context(View::Bytes route) const
    -> const Abstract& {
  return retained_monographs.visit(
      route,
      [](const Reference<Language::Monograph>& selected) -> const Abstract& {
        return selected.get();
      },
      []() -> const Abstract& { return Invalid::get_invalid(); });
}
