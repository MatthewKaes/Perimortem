// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/puffer/resolution/resolver.hpp"

#include "perimortem/core/algorithm/search.hpp"

#include "perimortem/memory/managed/bytes.hpp"

#include "perimortem/system/file.hpp"
#include "perimortem/system/path.hpp"

#include "tetrodotoxin/puffer/isa/boot/virtual_machine.hpp"
#include "ttx/lexical/cursor.hpp"
#include "ttx/lexical/tokenizer.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Perimortem::System;
using namespace Tetrodotoxin::Isa;
using namespace Tetrodotoxin::Puffer;

auto Resolution::Resolver::read_embedded(
    Allocator::Arena& arena,
    View::Bytes source,
    View::Bytes relative,
    View::Bytes& content) -> Bool {
  Path path(source, relative);
  if (!File::exists(path)) {
    return False;
  }

  Dynamic::Bytes file = File::read(path);
  Managed::Bytes retained(arena);
  retained.concat(file);
  content = retained.get_view();
  return True;
}

static auto blocked_contains(
    const Dynamic::Set<View::Bytes>* blocked_sources,
    View::Bytes key) -> Bool {
  if (blocked_sources == nullptr) {
    return False;
  }

  return blocked_sources->find(key) != nullptr;
}

static auto unresolved_import_message(
    View::Bytes import_name,
    Managed::Bytes& output) -> void {
  output.concat("Couldn't resolve import `"_view);
  output.concat(import_name);
  output.concat("`."_view);
}

auto Resolution::Resolver::resolve(View::Bytes key) -> Source::Record* {
  return sources.find(key);
}

auto Resolution::Resolver::find_package(View::Bytes package_name) const
    -> const Tetrodotoxin::Archiver::Package* {
  return packages.find(package_name);
}

auto Resolution::Resolver::load_source(
    Resolution::Context& context,
    View::Bytes source_path) -> Source::Record* {
  if (Algorithm::search(source_path, '\\') != Count(-1)) {
    context.persist_errors(
        Ttx::Lexical::Errors::Error(
            source_path, View::Bytes(),
            "File paths must use `/` separators."_view));
    return nullptr;
  }

  Path normalized_path(source_path);
  if (normalized_path.get_view().is_empty() || normalized_path.is_rooted()) {
    context.persist_errors(
        Ttx::Lexical::Errors::Error(
            source_path, View::Bytes(),
            "Source path is outside the source tree."_view));
    return nullptr;
  }

  if (!source_roots.include(normalized_path.get_view())) {
    context.persist_errors(
        Ttx::Lexical::Errors::Error(
            source_path, View::Bytes(),
            "Source path is outside the project source tree."_view));
    return nullptr;
  }

  Source::Record* cached = sources.find(normalized_path.get_view());
  if (cached != nullptr) {
    return cached;
  }

  // The disk entry point has the same contract as memory loading: return a
  // valid record with its imports resolved, or leave no public cache entry.
  Dynamic::Bytes source_text = File::read(normalized_path.get_view());
  if (source_text.is_empty()) {
    context.persist_errors(
        Ttx::Lexical::Errors::Error(
            normalized_path.get_view(), View::Bytes(),
            "Imported source file could not be read."_view));
    return nullptr;
  }

  Dynamic::Set<View::Bytes> active_includes;
  return load_source(
      context, normalized_path.get_view(), source_text.get_view(),
      active_includes);
}

auto Resolution::Resolver::load_source(
    Resolution::Context& context,
    View::Bytes source_path,
    View::Bytes ttx_content) -> Source::Record* {
  if (Algorithm::search(source_path, '\\') != Count(-1)) {
    context.persist_errors(
        Ttx::Lexical::Errors::Error(
            source_path, ttx_content,
            "File paths must use `/` separators."_view));
    return nullptr;
  }

  Path normalized_path(source_path);
  if (normalized_path.get_view().is_empty() || normalized_path.is_rooted()) {
    context.persist_errors(
        Ttx::Lexical::Errors::Error(
            source_path, ttx_content,
            "Source path is outside the source tree."_view));
    return nullptr;
  }

  if (!source_roots.include(normalized_path)) {
    context.persist_errors(
        Ttx::Lexical::Errors::Error(
            source_path, ttx_content,
            "Source path is outside the project source tree."_view));
    return nullptr;
  }

  Dynamic::Set<View::Bytes> active_includes;
  return load_source(
      context, normalized_path.get_view(), ttx_content, active_includes);
}

auto Resolution::Resolver::register_package_buffer(
    Resolution::Context& context,
    View::Bytes buffer_path,
    View::Bytes content) -> Bool {
  return packages.register_buffer(context, buffer_path, content);
}

auto Resolution::Resolver::reset() -> void {
  sources.reset();
  source_roots.reset();
  packages.reset();
}

auto Resolution::Resolver::load_source(
    Resolution::Context& context,
    View::Bytes source_path,
    View::Bytes ttx_content,
    Dynamic::Set<View::Bytes>& active_includes,
    const Dynamic::Set<View::Bytes>* blocked_sources) -> Source::Record* {
  Dynamic::Vector<Source::Snapshot> snapshots;
  Source::Record* existing = sources.find(source_path);
  if (existing != nullptr && existing->get_content() == ttx_content) {
    return existing;
  }

  if (existing != nullptr) {
    // Replacing a source may invalidate every source that imports it. Keep the
    // old consumer source text long enough to re-evaluate those consumers after
    // this source publishes its new record.
    snapshot_consumers(*existing, snapshots);
    sources.remove(*existing);
  }

  Dynamic::Object<Source::Record> candidate_handle(source_path, ttx_content);
  Source::Record& candidate = *candidate_handle;
  Ttx::Lexical::Tokenizer tokenizer(
      candidate.get_arena(), candidate.get_content(),
      candidate.get_source_path());
  Ttx::Lexical::Cursor cursor(tokenizer, candidate.get_arena());
  Isa::Boot::Envelope* boot = evaluate_boot(cursor);
  if (boot == nullptr || cursor.get_errors().has_errors()) {
    context.persist_errors(cursor.get_errors());
    return nullptr;
  }

  View::Bytes include_key = candidate.get_source_path();
  if (active_includes.contains(include_key)) {
    cursor.error("Import cycle detected while resolving source graph."_view);
    context.persist_errors(cursor.get_errors());
    return nullptr;
  }

  // Active includes are the depth-first path, not durable cache state. A source
  // or package only publishes after every import under it has resolved.
  active_includes.insert(include_key);
  Dynamic::Vector<Source::Record*> producers;
  Bool resolved = resolve_imports(
      context, cursor, candidate.get_source_path(), *boot, producers,
      active_includes, blocked_sources);
  active_includes.remove(include_key);
  if (!resolved || cursor.get_errors().has_errors()) {
    context.persist_errors(cursor.get_errors());
    return nullptr;
  }

  const auto* dialect = isa_registry.find(boot->get_isa());
  if (dialect == nullptr) {
    cursor.error("Selected ISA is not installed in this registry."_view);
    context.persist_errors(cursor.get_errors());
    return nullptr;
  }

  Ttx::Type* type =
      execute_body(cursor, candidate, *dialect, *boot, producers.get_view());
  if (type == nullptr && cursor.get_errors().is_empty()) {
    cursor.error("Selected ISA did not produce a TTX type."_view);
  }

  if (type == nullptr || cursor.get_errors().has_errors()) {
    context.persist_errors(cursor.get_errors());
    return nullptr;
  }

  if (!candidate.complete(*dialect, boot->get_imports(), *type)) {
    cursor.error("Resolved source record could not be completed."_view);
    context.persist_errors(cursor.get_errors());
    return nullptr;
  }

  Dynamic::Object<Source::Record> record_handle =
      context.adopt_record(candidate_handle);
  Source::Record& record = *record_handle;
  existing = sources.find(record.get_source_path());
  if (existing != nullptr) {
    // If the source key is already cached, the old record must leave with its
    // consumer tree before this record can publish.
    snapshot_consumers(*existing, snapshots);
    sources.remove(record.get_source_path());
  }

  // Publishing is the commit point. From here the cache may answer `resolve`
  // for this source path, and dependency indexes can point at it.
  // Resolving a package, even if it's valid, before it's published results in a
  // page fault for the evaluating machine which means cycle detection has to
  // happen during cascading Boot since it would just infinitely Boot loop on
  // imports otherwise.
  if (!sources.publish(record_handle)) {
    return nullptr;
  }

  for (Count i = 0; i < producers.get_size(); i++) {
    sources.connect(record, *producers[i]);
  }

  update_consumers(context, snapshots);
  return &record;
}

auto Resolution::Resolver::load_import(
    Resolution::Context& context,
    Ttx::Lexical::Cursor& cursor,
    View::Bytes owner_source_path,
    const Isa::Boot::Import& import,
    Dynamic::Set<View::Bytes>& active_includes,
    const Dynamic::Set<View::Bytes>* blocked_sources) -> Source::Record* {
  if (import.is_package()) {
    const auto* dialect = isa_registry.find("Package"_view);
    if (dialect == nullptr) {
      cursor.error("Imported package ISA is not installed."_view);
      return nullptr;
    }

    return packages.load(
        context, cursor, sources, *dialect, import.get_source_name(),
        active_includes);
  }

  // File imports stay inside the normalized project tree. Package imports
  // restore prebuilt buffers from the separate package repository and never
  // guess at source locations here.
  if (Algorithm::search(import.get_source_name(), '\\') != Count(-1)) {
    cursor.error("File paths must use `/` separators."_view);
    return nullptr;
  }

  Path target_path(owner_source_path, import.get_source_name());
  if (target_path.get_view().is_empty() || target_path.is_rooted() ||
      !source_roots.contains(target_path.get_view())) {
    cursor.error("Import source path is outside the source tree."_view);
    return nullptr;
  }

  if (blocked_contains(blocked_sources, target_path.get_view())) {
    Managed::Bytes message(cursor.get_arena());
    unresolved_import_message(target_path.get_view(), message);
    cursor.error(message);
    return nullptr;
  }

  Source::Record* cached = sources.find(target_path.get_view());
  if (cached != nullptr) {
    return cached;
  }

  if (active_includes.contains(target_path.get_view())) {
    cursor.error("Import cycle detected while resolving source graph."_view);
    return nullptr;
  }

  Dynamic::Bytes source_text = File::read(target_path.get_view());
  if (source_text.is_empty()) {
    context.persist_errors(
        Ttx::Lexical::Errors::Error(
            target_path.get_view(), View::Bytes(),
            "Imported source file could not be read."_view));
    return nullptr;
  }

  load_source(
      context, target_path.get_view(), source_text.get_view(), active_includes,
      blocked_sources);
  return sources.find(target_path.get_view());
}

auto Resolution::Resolver::evaluate_boot(Ttx::Lexical::Cursor& cursor)
    -> Isa::Boot::Envelope* {
  // Resolver calls Puffer Boot directly for complete source files and passes
  // the caller's body ISA registry so the preamble can validate the selected
  // ISA and import ISA operands.
  return Isa::Boot::VirtualMachine::evaluate(cursor, isa_registry);
}

auto Resolution::Resolver::execute_body(
    Ttx::Lexical::Cursor& cursor,
    Source::Record& record,
    const Tetrodotoxin::Isa::Dialect& dialect,
    const Isa::Boot::Envelope& boot,
    View::Vector<Source::Record*> producers) -> Ttx::Type* {
  // Body ISAs execute after Boot has selected the instruction set and imports
  // are satisfied. That gives ISAs like Package the correct staging point to
  // bind authored references to real Ttx::Type addresses instead of inventing a
  // temporary semantic tree.
  Tetrodotoxin::Isa::Base::Context isa_context(
      cursor.get_arena(), &record.get_implementation(), &read_embedded);
  isa_context.set_package_name(package_name);
  isa_context.set_module(record.get_module());

  auto imports = boot.get_imports();
  for (Count i = 0; i < producers.get_size(); i++) {
    const Ttx::Type& type = producers[i]->get_type();
    if (!isa_context.define_type(imports[i].get_local_name(), type)) {
      cursor.error("Import type name is already defined."_view);
      return nullptr;
    }

    isa_context.import_implementation(producers[i]->get_implementation());
  }

  return dialect.get_evaluator()(cursor, isa_context);
}

auto Resolution::Resolver::resolve_imports(
    Resolution::Context& context,
    Ttx::Lexical::Cursor& cursor,
    View::Bytes owner_source_path,
    const Isa::Boot::Envelope& boot,
    Dynamic::Vector<Source::Record*>& producers,
    Dynamic::Set<View::Bytes>& active_includes,
    const Dynamic::Set<View::Bytes>* blocked_sources) -> Bool {
  Bool resolved = True;

  auto requested_imports = boot.get_imports();
  // Evaluate every requested import so a single load reports as much of the
  // source error surface as possible. Producers are connected only after the
  // whole import list succeeds.
  for (Count i = 0; i < requested_imports.get_size(); i++) {
    const Isa::Boot::Import& import = requested_imports[i];
    Source::Record* producer = load_import(
        context, cursor, owner_source_path, import, active_includes,
        blocked_sources);
    if (producer == nullptr) {
      resolved = False;
      continue;
    }

    if (import.get_isa() != producer->get_dialect().get_name()) {
      cursor.error(
          "Imported source ISA does not match the requested ISA."_view);
      resolved = False;
      continue;
    }

    producers.insert(producer);
  }

  if (!resolved) {
    return False;
  }

  return True;
}

auto Resolution::Resolver::update_consumers(
    Resolution::Context& context,
    Dynamic::Vector<Source::Snapshot>& snapshots) -> void {
  Dynamic::Set<View::Bytes> blocked_sources;
  // Consumers are replayed from their old source text after a dependency
  // changes. If one fails, later consumers should still be checked, but they
  // must treat that failed source as unavailable instead of accidentally using
  // a stale record from earlier in the update.
  for (Count i = 0; i < snapshots.get_size(); i++) {
    Dynamic::Set<View::Bytes> active_includes;
    if (load_source(
            context, snapshots[i].get_source_path(),
            snapshots[i].get_source_text(), active_includes,
            &blocked_sources) == nullptr) {
      blocked_sources.insert(snapshots[i].get_source_path());
    }
  }
}

auto Resolution::Resolver::snapshot_consumers(
    Source::Record& record,
    Dynamic::Vector<Source::Snapshot>& snapshots) -> void {
  Dynamic::Vector<Source::Record*> consumers;
  sources.collect_consumers(record, consumers);
  // Cache removal releases cache ownership, so consumer update work is captured
  // as path and source text snapshots before the dependency tree is removed.
  for (Count i = 0; i < consumers.get_size(); i++) {
    snapshots.insert(
        Source::Snapshot(
            consumers[i]->get_source_path(), consumers[i]->get_content()));
  }
}
