// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/environment/workspace.hpp"

#include "perimortem/core/diagnostics/log.hpp"

#include "perimortem/memory/dynamic/object.hpp"

#include "perimortem/system/path.hpp"

#include "tetrodotoxin/package/content.hpp"
#include "tetrodotoxin/package/storage.hpp"
#include "ttx/concept/invalid.hpp"
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

Environment::Workspace::Workspace()
    : arena(),
      dialects(arena),
      transactions(),
      source_monographs(arena),
      packages(arena) {}

Environment::Workspace::~Workspace() = default;

auto Environment::Workspace::interpret_source(
    Errors& errors,
    View::Bytes semantic_name,
    View::Bytes diagnostic_path,
    View::Bytes contents) -> Option<Language::Monograph&> {
  // Source backed graph objects retain views into this candidate Arena. Keeping
  // the complete lexical and semantic transaction under one handle makes every
  // rejection release those views as one lifetime decision.
  Dynamic::Object<Allocator::Arena> transaction;
  View::Bytes retained_contents = transaction->proxy(contents);
  View::Bytes retained_path = transaction->proxy(diagnostic_path);
  Tokenizer& tokenizer = transaction->construct<Tokenizer>(
      *transaction, retained_contents, retained_path);
  Cursor& cursor = transaction->construct<Cursor>(tokenizer, errors);

  if (source_monographs.contains(semantic_name)) {
    cursor.create_error(
        "This semantic source name is already published in the Workspace."_view,
        semantic_name);
    return {};
  }

  Count source_error_count = errors.get_size();
  auto monograph = Language::Dialect::interpret_source(
      dialects.get_dialects(), cursor, *this);
  if (!monograph) {
    if (errors.get_size() == source_error_count) {
      cursor.create_error(
          "Source interpretation failed without a more specific diagnostic."_view);
    }
    return {};
  }

  if (monograph->is<Package::Language::Monograph>()) {
    // Package completion needs its fixed member barrier. Letting the direct path
    // retain a manifest would publish aliases before their member owners exist.
    cursor.create_error(
        "A Package manifest must be completed through Workspace Package "
        "import."_view);
    return {};
  }

  source_error_count = errors.get_size();
  if (!monograph->link(cursor)) {
    if (errors.get_size() == source_error_count) {
      cursor.create_error(
          "Source linking failed without a more specific diagnostic."_view);
    }
    return {};
  }

  source_error_count = errors.get_size();
  if (!monograph->finalize(cursor)) {
    if (errors.get_size() == source_error_count) {
      cursor.create_error(
          "Source finalization failed without a more specific diagnostic."_view);
    }
    return {};
  }

  // Publication follows complete source semantics. Until this point the
  // Workspace has no lookup edge or retained Arena for the candidate graph.
  transactions.insert(transaction);
  View::Bytes retained_name = arena.proxy(semantic_name);
  source_monographs.launder(retained_name, *monograph);
  return *monograph;
}

auto Environment::Workspace::import_package(
    Errors& errors,
    View::Bytes package_root,
    View::Bytes root_semantic_name,
    View::Bytes root_logical_route,
    View::Bytes root_package_identity,
    Version root_package_version) -> Option<Language::Monograph&> {
  // Storage has no authored Cursor until the manifest is read. Root and
  // manifest acquisition failures stay in process diagnostics, while every
  // later failure uses the matching source Cursor.
  Allocator::Arena acquisition;

  auto storage = Package::Storage::open(acquisition, package_root);
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
  // Package Monograph share one Arena so any rejection releases them together.
  Dynamic::Object<Allocator::Arena> root_transaction;
  View::Bytes root_contents = root_transaction->proxy(manifest->get_contents());
  View::Bytes root_path =
      root_transaction->proxy(manifest->get_diagnostic_path());
  Tokenizer& root_tokenizer = root_transaction->construct<Tokenizer>(
      *root_transaction, root_contents, root_path);
  Cursor& root_cursor =
      root_transaction->construct<Cursor>(root_tokenizer, errors);
  if (source_monographs.contains(root_semantic_name)) {
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

  // Package interpretation establishes the complete Dependency and Source
  // description table before Workspace acquires any member.
  Count root_error_count = errors.get_size();
  auto root_owner = Language::Dialect::interpret_source(
      dialects.get_dialects(), root_cursor, *this);
  if (!root_owner) {
    if (errors.get_size() == root_error_count) {
      root_cursor.create_error(
          "Package manifest interpretation failed without a more specific "
          "diagnostic."_view);
    }
    return {};
  }

  auto selected_root = root_owner->select<Package::Language::Monograph>();
  if (!selected_root) {
    root_cursor.create_error(
        "The root source of a Package import must use the installed Package "
        "Dialect."_view);
    return {};
  }
  Package::Language::Monograph& root = *selected_root;

  // Package resources borrow this import's confined Storage only while sources
  // parse. Sealing before linking removes that physical capability.
  if (!root.get_resources().connect(*storage)) {
    root_cursor.create_error(
        "The Package resource table rejected its one import storage."_view);
    return {};
  }

  // Dependencies are completed Workspace facts, not nested import requests.
  // Exact identity and version matching keeps this transaction's scope fixed.
  for (const Package::Language::Dependency& dependency :
       root.get_dependencies()) {
    const ImportedPackage* selected = nullptr;
    for (Count i = 0; i < packages.get_size(); i++) {
      const ImportedPackage& imported = packages[i];
      if (imported.identity == dependency.get_package_name()) {
        selected = &imported;
        break;
      }
    }

    if (selected == nullptr) {
      root_cursor.create_expression_error(
          dependency.get_span(),
          "Package dependency is not already imported in this Workspace."_view,
          dependency.get_package_name());
      root.get_resources().seal();
      return {};
    }

    if (selected->version != dependency.get_version()) {
      root_cursor.create_expression_error(
          dependency.get_span(),
          "Package dependency requests a different version than the one "
          "already imported in this Workspace."_view,
          dependency.get_package_name());
      root.get_resources().seal();
      return {};
    }

    if (!root.bind_dependency(dependency, *selected->monograph)) {
      root_cursor.create_expression_error(
          dependency.get_span(),
          "Package dependency could not enter the Package mapping table."_view,
          dependency.get_local_name());
      root.get_resources().seal();
      return {};
    }
  }

  // Workspace keeps every candidate Arena local while Package records only
  // borrowed mappings. An early return destroys the complete candidate set.
  Dynamic::Vector<Dynamic::Object<Allocator::Arena>> candidate_transactions(
      root.get_sources().get_size() + 1);
  Managed::Vector<Language::Monograph*> candidates(acquisition);
  Managed::Vector<Cursor*> cursors(acquisition);
  candidate_transactions.insert(root_transaction);
  candidates.insert(&root);
  cursors.insert(&root_cursor);

  // Each declared Source gets its own owner and Cursor so source backed values
  // and diagnostics retain the member's exact text and location.
  Bool parsed = True;
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

    Dynamic::Object<Allocator::Arena> source_transaction;
    View::Bytes source_contents =
        source_transaction->proxy(content->get_contents());
    View::Bytes source_path =
        source_transaction->proxy(content->get_diagnostic_path());
    Tokenizer& tokenizer = source_transaction->construct<Tokenizer>(
        *source_transaction, source_contents, source_path);
    Cursor& cursor = source_transaction->construct<Cursor>(tokenizer, errors);
    Count source_error_count = errors.get_size();
    auto member = Language::Dialect::interpret_source(
        dialects.get_dialects(), cursor, root);
    if (!member) {
      if (errors.get_size() == source_error_count) {
        cursor.create_error(
            "Package source interpretation failed without a more specific "
            "diagnostic."_view);
      }
      parsed = False;
      continue;
    }

    // The root manifest already fixed the complete table. Accepting a Package
    // member here would recursively grow that table outside this barrier.
    if (member->is<Package::Language::Monograph>()) {
      cursor.create_error(
          "A Package Source cannot create another Package import."_view);
      parsed = False;
      continue;
    }

    if (!root.bind_member(source.get_local_route(), *member)) {
      root_cursor.create_expression_error(
          source.get_span(),
          "Package source could not enter the Package mapping table."_view,
          source.get_local_name());
      parsed = False;
      continue;
    }

    candidate_transactions.insert(source_transaction);
    candidates.insert(&*member);
    cursors.insert(&cursor);
  }
  // Source parsing is the only stage with storage access. Linking observes a
  // sealed Package context whose semantic candidates can no longer expand.
  root.get_resources().seal();

  if (!parsed) {
    return {};
  }

  // Every parse valid identity must exist before any member resolves context.
  // This keeps authored Source order from deciding which routes are visible.
  Bool linked = True;
  for (Count i = 0; i < candidates.get_size(); i++) {
    Count source_error_count = errors.get_size();
    if (!candidates[i]->link(*cursors[i])) {
      if (errors.get_size() == source_error_count) {
        cursors[i]->create_error(
            "Package source linking failed without a more specific "
            "diagnostic."_view);
      }
      linked = False;
    }
  }
  if (!linked) {
    return {};
  }

  // Finalization may consume linked declarations from any member, so no
  // candidate enters it until the whole graph links.
  Bool finalized = True;
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
  if (!finalized) {
    return {};
  }

  // Retaining all Arenas is the transaction commit. Package aliases become
  // durable only with their owners, and every failure above publishes nothing.
  for (Count i = 0; i < candidate_transactions.get_size(); i++) {
    transactions.insert(candidate_transactions[i]);
  }

  View::Bytes retained_identity = arena.proxy(root_package_identity);
  packages.insert({
    .identity = retained_identity,
    .version = root_package_version,
    .monograph = &root,
  });
  View::Bytes retained_name = arena.proxy(root_semantic_name);
  source_monographs.launder(retained_name, root);
  return root;
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
  return source_monographs.visit(
      route,
      [](const Language::Monograph& selected) -> const Abstract& {
        return selected;
      },
      []() -> const Abstract& { return Invalid::get_invalid(); });
}
