// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/environment/workspace.hpp"

#include "perimortem/core/diagnostics/log.hpp"

#include "tetrodotoxin/language/parser/comment.hpp"
#include "tetrodotoxin/language/parser/dialect.hpp"
#include "tetrodotoxin/package/language/monograph.hpp"
#include "tetrodotoxin/package/storage.hpp"
#include "ttx/concept/invalid.hpp"
#include "ttx/lexical/span.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Perimortem::Utility;
using namespace Ttx::Concept;
using namespace Ttx::Lexical;
using namespace Tetrodotoxin;

static constexpr View::Bytes package_import_operation =
    "Environment::Workspace Package import"_view;

struct StagedSource {
  View::Bytes semantic_name;
  View::Bytes logical_route;
};

auto Environment::Workspace::import_source(
    View::Bytes semantic_name,
    View::Bytes diagnostic_path,
    View::Bytes contents,
    Errors& errors) -> Option<Language::Dialect::Monograph&> {
  // A raw View carries no storage provenance. Direct callers may supply stack,
  // Dynamic, mapped, or another Arena's bytes, while every parser product and
  // Monograph is allowed to borrow this source for the Workspace lifetime.
  // Copy all three views exactly once before entering the retained transaction.
  View::Bytes retained_semantic_name = arena.proxy(semantic_name);
  View::Bytes retained_diagnostic_path = arena.proxy(diagnostic_path);
  View::Bytes retained_contents = arena.proxy(contents);

  return import_retained_source(
      retained_semantic_name, retained_diagnostic_path, retained_contents,
      errors);
}

auto Environment::Workspace::import_retained_source(
    View::Bytes semantic_name,
    View::Bytes diagnostic_path,
    View::Bytes contents,
    Errors& errors) -> Option<Language::Dialect::Monograph&> {
  // This is the single semantic import transaction. Its three Views must
  // already live in the Workspace Arena: public import_source establishes that
  // invariant by copying, while import_package receives them from Arena backed
  // Package Monographs and Package Storage. Keeping allocation policy outside
  // this body prevents staged file bytes from being duplicated solely because
  // View cannot describe its owner.

  // An existing semantic identity is never reparsed or overwritten. Errors
  // snapshots the retained input before this transaction ends.
  if (source_monographs.contains(semantic_name)) {
    Errors::Report report(errors, diagnostic_path, contents, Span());
    report << "Semantic source "_view << semantic_name
           << " is already imported into the Workspace."_view;
    return {};
  }

  // Parse the universal source envelope from the retained source body. Cursor
  // installs the separate diagnostic path on Errors without turning that path
  // into semantic identity.
  Tokenizer tokenizer(arena, contents, diagnostic_path);
  Cursor cursor(tokenizer, errors);

  // Every source begins with authored Documentation. Absence is a parse
  // failure and creates no semantic publication.
  if (cursor.get_code() != Code::Type::Comment) {
    cursor.require(
        Code::Type::Comment,
        "Source is missing required documentation comment. Provide at least an "
        "explicit empty comment."_view);
    return {};
  }

  // Parse the top level Documentation and dispatch the exact authored Dialect
  // name from the same retained source.
  const Documentation& documentation = Language::Parser::Comment::parse(cursor);
  Token dialect_instruction = cursor.current();
  View::Bytes dialect_name = Language::Parser::Dialect::parse(cursor);
  if (dialect_name.is_empty()) {
    return {};
  }

  // Locate the Dialect in installation order and report the complete authored
  // name inventory when the source requests an unavailable owner.
  auto* dialect_entry = dialects.find(dialect_name);
  if (dialect_entry == nullptr) {
    Errors::Report report(
        errors, diagnostic_path, contents, Span(dialect_instruction));
    auto& hint = report.get_hint();
    const Count installed_count = installed_names.get_size();

    report << "Unknown dialect "_view << dialect_name
           << " can't be used to interpret this source."_view;

    hint << "Installed dialects: "_view;
    if (installed_count == 0) {
      hint << "<None>"_view;
    } else {
      for (Count i = 0; i < installed_count; i++) {
        if (i != 0) {
          hint << ", "_view;
        }

        hint << installed_names[i];
      }
    }

    hint << "."_view;
    return {};
  }

  // The concrete Dialect constructs its real Monograph in the Workspace Arena
  // and can retain the same source views for the semantic island lifetime.
  Option<Language::Dialect::Monograph&> interpreted =
      dialect_entry->value.interpret(arena, cursor, documentation, *this);
  if (!interpreted) {
    return {};
  }

  // Retain the completed Monograph exactly once, then publish its exact
  // semantic name. Only successful interpretation reaches this mutation.
  Language::Dialect::Monograph& monograph = *interpreted;
  if (!retained_monographs.contains(&monograph)) {
    retained_monographs.insert(&monograph);
  }

  source_monographs.launder(semantic_name, monograph);
  return monograph;
}

Environment::Workspace::Workspace()
    : arena(),
      installed_names(arena),
      installed_dialects(arena),
      retained_monographs(arena),
      dialects(arena),
      source_monographs(arena) {}

Environment::Workspace::~Workspace() {
  // Monographs may retain both their host Dialect and Arena backed semantic
  // facts, so finish their complete destruction phase first.
  for (Count i = 0; i < retained_monographs.get_size(); i++) {
    retained_monographs[i]->~Monograph();
  }

  // Dialect state remains usable until every hosted Monograph is gone. The
  // Arena is the first member and therefore releases its pages last.
  for (Count i = 0; i < installed_dialects.get_size(); i++) {
    installed_dialects[i]->~Dialect();
  }
}

auto Environment::Workspace::import_package(
    View::Bytes package_root,
    View::Bytes root_semantic_name,
    View::Bytes root_logical_route,
    Ttx::Lexical::Errors& errors) -> Option<Language::Dialect::Monograph&> {
  auto storage = Package::Storage::open(arena, package_root);
  if (!storage) {
    Diagnostics::Log::Message<512> message(Diagnostics::Log::Level::Info);
    message << package_import_operation
            << " failed. reason=the Package root could not be opened "
               "package_root="_view
            << package_root << " root_semantic_name="_view << root_semantic_name
            << " root_logical_route="_view << root_logical_route;
    return {};
  }

  // The FIFO itself is transaction state, but every record borrows values in
  // the Workspace Arena. Root caller values cross that lifetime boundary once.
  Managed::Vector<StagedSource> staged_sources(arena);
  StagedSource root = {
    .semantic_name = arena.proxy(root_semantic_name),
    .logical_route = arena.proxy(root_logical_route),
  };
  staged_sources.insert(root);

  Package::Storage& package_storage = *storage;
  Option<Language::Dialect::Monograph&> root_monograph;
  Count next_source = 0;
  Bool failed = False;

  // A monotonic index preserves breadth first authored order. No physical
  // member is discovered here: only a real Package Monograph appends Sources.
  while (next_source < staged_sources.get_size()) {
    StagedSource staged = staged_sources[next_source];
    next_source++;

    Option<Package::Storage::Content&> content =
        package_storage.read(staged.logical_route);
    if (!content) {
      Diagnostics::Log::Message<512> message(Diagnostics::Log::Level::Info);
      message << package_import_operation
              << " failed. reason=the staged semantic source could not be read "
                 "semantic_name="_view
              << staged.semantic_name << " logical_route="_view
              << staged.logical_route;
      failed = True;
      continue;
    }

    // Every argument has proven Workspace lifetime. Staged names are either the
    // retained root name or a view held by a Package Monograph. Storage was
    // opened with this same Arena, so Content retains both its normalized
    // diagnostic path and its file bytes here. Entering the semantic
    // transaction directly avoids allocating and copying the complete source
    // a second time.
    Option<Language::Dialect::Monograph&> imported = import_retained_source(
        staged.semantic_name, (*content).get_diagnostic_path(),
        (*content).get_contents(), errors);
    if (!imported) {
      failed = True;
      continue;
    }

    if (next_source == 1) {
      root_monograph = imported;
    }

    Language::Dialect::Monograph& monograph = *imported;
    if (!monograph.is<Package::Language::Monograph>()) {
      continue;
    }

    const auto& package =
        static_cast<const Package::Language::Monograph&>(monograph);
    View::Vector<Package::Language::Source> sources = package.get_sources();
    for (Count i = 0; i < sources.get_size(); i++) {
      StagedSource member = {
        .semantic_name = sources[i].get_local_name(),
        .logical_route = sources[i].get_source_path(),
      };
      staged_sources.insert(member);
    }
  }

  if (failed) {
    return {};
  }

  return root_monograph;
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
