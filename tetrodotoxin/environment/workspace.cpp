// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/environment/workspace.hpp"

#include "perimortem/core/diagnostics/log.hpp"

#include "tetrodotoxin/language/parser/comment.hpp"
#include "tetrodotoxin/language/parser/dialect.hpp"
#include "tetrodotoxin/package/content.hpp"
#include "tetrodotoxin/package/storage.hpp"
#include "ttx/concept/invalid.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Perimortem::System;
using namespace Perimortem::Utility;
using namespace Ttx::Concept;
using namespace Ttx::Lexical;
using namespace Tetrodotoxin;

static constexpr View::Bytes package_import_operation =
    "Environment::Workspace Package import"_view;

static constexpr auto storage_failure_error_name(
    Package::Storage::Failure::Error error) -> View::Bytes {
  switch (error) {
  case Package::Storage::Failure::Error::InvalidRoute:
    return "InvalidRoute"_view;
  case Package::Storage::Failure::Error::Unreadable:
    return "Unreadable"_view;
  default:
    return "Unknown"_view;
  }
}

static_assert(
    storage_failure_error_name(Package::Storage::Failure::Error::Unknown) ==
    "Unknown"_view);
static_assert(
    storage_failure_error_name(
        static_cast<Package::Storage::Failure::Error>(Unsigned_8(-2))) ==
    "Unknown"_view);

struct StagedSource {
  View::Bytes semantic_name;
  View::Bytes logical_route;
  Option<Package::Language::Monograph&> owner;
  Bool publish_globally;
};

Environment::Workspace::Workspace()
    : arena(),
      dialects(arena, *this),
      retention(arena),
      resolution(arena, dialects, retention),
      source_monographs(arena) {}

Environment::Workspace::~Workspace() = default;

auto Environment::Workspace::import_source(
    Errors& errors,
    View::Bytes semantic_name,
    View::Bytes diagnostic_path,
    View::Bytes contents) -> Option<Language::Dialect::Monograph&> {
  // A raw View carries no storage provenance. Direct callers may supply stack,
  // Dynamic, mapped, or another Arena's bytes, while parser products borrow
  // source text for the Workspace lifetime. Copy each view at this boundary.
  View::Bytes retained_semantic_name = arena.proxy(semantic_name);
  View::Bytes retained_diagnostic_path = arena.proxy(diagnostic_path);
  View::Bytes retained_contents = arena.proxy(contents);

  return import_retained_source(
      errors, retained_semantic_name, retained_diagnostic_path,
      retained_contents, *this, True);
}

auto Environment::Workspace::import_retained_source(
    Errors& errors,
    View::Bytes semantic_name,
    View::Bytes diagnostic_path,
    View::Bytes contents,
    Abstract& interpretation_context,
    Bool publish_globally) -> Option<Language::Dialect::Monograph&> {
  // Public import copies arbitrary caller views. Package Storage already lives
  // in the same Arena and enters here directly, avoiding a duplicate body for
  // every staged member.
  if (publish_globally && source_monographs.contains(semantic_name)) {
    Errors::Report report(errors, diagnostic_path, contents, Span());
    report << "Semantic source "_view << semantic_name
           << " is already imported into the Workspace."_view;
    return {};
  }

  // Cursor installs the diagnostic path while semantic identity remains the
  // separate exact authored name supplied by the transaction.
  Tokenizer tokenizer(arena, contents, diagnostic_path);
  Cursor cursor(tokenizer, errors);
  if (cursor.get_code() != Code::Type::Comment) {
    cursor.require(
        Code::Type::Comment,
        "Source is missing required documentation comment. Provide at least an "
        "explicit empty comment."_view);
    return {};
  }

  const Documentation& documentation = Language::Parser::Comment::parse(cursor);
  Token dialect_instruction = cursor.current();
  View::Bytes dialect_name = Language::Parser::Dialect::parse(cursor);
  if (dialect_name.is_empty()) {
    return {};
  }

  Option<Language::Dialect&> dialect = dialects.find(dialect_name);
  if (!dialect) {
    Errors::Report report(
        errors, diagnostic_path, contents, Span(dialect_instruction));
    auto& hint = report.get_hint();
    View::Vector<View::Bytes> installed_names = dialects.get_names();

    report << "Unknown dialect "_view << dialect_name
           << " can't be used to interpret this source."_view;
    hint << "Installed dialects: "_view;
    if (installed_names.is_empty()) {
      hint << "<None>"_view;
    } else {
      for (Count i = 0; i < installed_names.get_size(); i++) {
        if (i != 0) {
          hint << ", "_view;
        }

        hint << installed_names[i];
      }
    }
    hint << "."_view;
    return {};
  }

  // The installed Dialect keeps Workspace as its shared registry. This
  // argument instead selects the exact source scope the body is entering.
  Option<Language::Dialect::Monograph&> interpreted =
      dialect->interpret(arena, cursor, documentation, interpretation_context);
  if (!interpreted) {
    return {};
  }

  // Retention owns lifetime and exact authored provenance. Package local
  // publication stays separate so its real Package can bind the Monograph
  // without adding the same local name to Workspace lookup.
  Language::Dialect::Monograph& monograph = *interpreted;
  Origin origin(diagnostic_path, contents, Span());
  retention.retain(monograph, origin);

  if (publish_globally) {
    source_monographs.launder(semantic_name, monograph);
  }

  return monograph;
}

auto Environment::Workspace::import_package(
    Errors& errors,
    View::Bytes package_root,
    View::Bytes root_semantic_name,
    View::Bytes root_logical_route,
    View::Bytes root_package_identity,
    Version root_package_version,
    Package::Repository::Repository& repository) -> Static::
    Union<Language::Dialect::Monograph&, Package::Repository::SelectionError> {
  auto storage = Package::Storage::open(arena, package_root);
  if (!storage) {
    Diagnostics::Log::Message<512> message(Diagnostics::Log::Level::Info);
    message << package_import_operation
            << " failed. reason=the Package root could not be opened "
               "package_root="_view
            << package_root << " root_semantic_name="_view << root_semantic_name
            << " root_logical_route="_view << root_logical_route
            << " root_package_identity="_view << root_package_identity
            << " root_package_version="_view << root_package_version.get_major()
            << '.' << root_package_version.get_minor();
    return {};
  }

  // Root caller values cross into Workspace lifetime once. Repository may use
  // another Arena and remains borrowed only during Resolution.
  Managed::Vector<StagedSource> staged_sources(arena);
  StagedSource root = {
    .semantic_name = arena.proxy(root_semantic_name),
    .logical_route = arena.proxy(root_logical_route),
    .owner = {},
    .publish_globally = True,
  };
  staged_sources.insert(root);

  View::Bytes retained_root_identity = arena.proxy(root_package_identity);
  Package::Storage& package_storage = *storage;
  Option<Language::Dialect::Monograph&> root_monograph;
  const Count first_monograph = retention.get_size();
  Count next_source = 0;
  Bool failed = False;

  // A monotonic index preserves authored breadth first order. Only a concrete
  // Package Monograph can append its declared Sources to this queue.
  while (next_source < staged_sources.get_size()) {
    StagedSource staged = staged_sources[next_source];
    next_source++;

    auto reject_read = [&](Package::Storage::Failure::Error error)
        -> Option<Package::Content&> {
      Diagnostics::Log::Message<512> message(Diagnostics::Log::Level::Info);
      message << package_import_operation
              << " failed. reason=the staged semantic source could not be read "
                 "semantic_name="_view
              << staged.semantic_name << " logical_route="_view
              << staged.logical_route << " storage_error="_view
              << storage_failure_error_name(error);
      failed = True;
      return {};
    };
    auto read = package_storage.read(staged.logical_route);
    Option<Package::Content&> content = read.visit(
        [](Package::Content& selected) {
          return Option<Package::Content&>(selected);
        },
        [&](const Package::Storage::Failure& failure) {
          return reject_read(failure.get_error());
        });
    if (!content) {
      continue;
    }

    // Storage path and body already meet the retained source contract because
    // Storage was opened with this Workspace Arena. The existing owner is the
    // complete Package scope, while an ownerless root enters Workspace.
    Abstract& interpretation_context =
        staged.owner ? static_cast<Abstract&>(*staged.owner) : *this;
    Option<Language::Dialect::Monograph&> imported = import_retained_source(
        errors, staged.semantic_name, content->get_diagnostic_path(),
        content->get_contents(), interpretation_context,
        staged.publish_globally);
    if (!imported) {
      failed = True;
      continue;
    }

    if (next_source == 1) {
      root_monograph = imported;
    }

    // Only the root lacks an owning Package. Reusing the same owner for
    // interpretation and binding keeps nested scope exact without a second
    // lookup or copied context.
    Language::Dialect::Monograph& monograph = *imported;
    if (staged.owner) {
      Package::Language::Monograph& owner = *staged.owner;
      Bool bound = owner.bind_member(staged.semantic_name, monograph);
      if (!bound) {
        retention.find_origin(owner).visit(
            []() {},
            [&](const Origin& origin) {
              Errors::Report report(
                  errors, origin.get_path(), origin.get_body(),
                  origin.get_span());
              report << "Package member "_view << staged.semantic_name
                     << " could not bind to its owning Package."_view;
            });
        failed = True;
      }
    }

    if (!monograph.is<Package::Language::Monograph>()) {
      continue;
    }

    auto& package = static_cast<Package::Language::Monograph&>(monograph);
    Bool resources_connected = package.get_resources().connect(package_storage);
    if (!resources_connected) {
      Diagnostics::Log::Message<512> message(Diagnostics::Log::Level::Info);
      message << package_import_operation
              << " failed. reason=Package resources could not connect "
                 "semantic_name="_view
              << staged.semantic_name;
      failed = True;
      continue;
    }

    View::Vector<Package::Language::Source> sources = package.get_sources();
    for (Count i = 0; i < sources.get_size(); i++) {
      StagedSource member = {
        .semantic_name = sources[i].get_local_name(),
        .logical_route = sources[i].get_source_path(),
        .owner = package,
        .publish_globally = False,
      };
      staged_sources.insert(member);
    }
  }

  // Every authored Package is retained in this discovery range before its
  // Sources enter the queue. Sealing the exact range here removes Storage from
  // every semantic owner before failure completion or dependency resolution.
  for (Count i = first_monograph; i < retention.get_size(); i++) {
    Language::Dialect::Monograph& retained = retention.get_monograph(i);
    if (!retained.is<Package::Language::Monograph>()) {
      continue;
    }

    auto& package = static_cast<Package::Language::Monograph&>(retained);
    package.get_resources().seal();
  }

  if (failed || !root_monograph ||
      !root_monograph->is<Package::Language::Monograph>()) {
    retention.complete(errors);
    return {};
  }

  // Resolution receives only a complete staged discovery range. Earlier source
  // failure has completed its retained prefix and cannot enter Archive
  // traversal or restored cache promotion.
  auto& root_package =
      static_cast<Package::Language::Monograph&>(*root_monograph);
  return resolution.resolve(
      errors, first_monograph, retained_root_identity, root_package_version,
      root_package, repository);
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
      [](const Language::Dialect::Monograph& selected) -> const Abstract& {
        return selected;
      },
      []() -> const Abstract& { return Invalid::get_invalid(); });
}
