// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/puffer/resolution/resolver.hpp"

#include "perimortem/core/data.hpp"
#include "perimortem/core/static/vector.hpp"

#include "perimortem/memory/managed/bytes.hpp"

#include "perimortem/system/file.hpp"
#include "perimortem/system/path.hpp"

#include "perimortem/utility/table.hpp"

#include "tetrodotoxin/isa/package/virtual_machine.hpp"
#include "tetrodotoxin/puffer/isa/boot/virtual_machine.hpp"
#include "ttx/lexical/cursor.hpp"
#include "ttx/lexical/tokenizer.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Perimortem::System;
using namespace Perimortem::Utility;
using namespace Tetrodotoxin::Isa;
using namespace Tetrodotoxin::Puffer::Isa;
using namespace Tetrodotoxin::Puffer::Resolution;

static constexpr View::Bytes standard_source_root =
    "tetrodotoxin/standard/"_view;

static constexpr Static::Vector<Pair<View::Bytes, View::Bytes>, 3>
    standard_package_roots = {{
      {
        "Perimortem::Graphics"_view,
        "tetrodotoxin/standard/perimortem/graphics/package.ttx"_view,
      },
      {
        "Perimortem::Math"_view,
        "tetrodotoxin/standard/perimortem/math/package.ttx"_view,
      },
      {
        "Perimortem::Runtime"_view,
        "tetrodotoxin/standard/perimortem/runtime/package.ttx"_view,
      },
    }};

using StandardPackageRoots = Table<View::Bytes, standard_package_roots>;

static auto starts_with(View::Bytes value, View::Bytes prefix) -> Bool {
  return value.get_size() >= prefix.get_size() &&
         value.slice(0, prefix.get_size()) == prefix;
}

static auto lowercase_ascii(Bits_8 value) -> Bits_8 {
  return value >= 'A' && value <= 'Z' ? value + ('a' - 'A') : value;
}

static auto append_package_segment(Dynamic::Bytes& output, View::Bytes segment)
    -> void {
  if (!output.get_view().is_empty()) {
    output.append('/');
  }

  for (Count byte_index = 0; byte_index < segment.get_size(); byte_index++) {
    output.append(lowercase_ascii(segment[byte_index]));
  }
}

static auto package_name_to_source_path(
    View::Bytes package_name,
    Dynamic::Bytes& output) -> Bool {
  output.clear();

  View::Bytes standard_root =
      StandardPackageRoots::find_or_default(package_name, View::Bytes());
  if (!standard_root.is_empty()) {
    output.concat(standard_root);
    return output.get_size() <= Path::max_size;
  }

  Count segment_start = 0;
  Count segment_count = 0;

  // User package names use symbol spelling at the source surface, but package
  // roots are regular source paths. Walk the package name one segment at a time
  // so `User::Core` naturally becomes `user/core/package.ttx`.
  for (Count name_index = 0; name_index <= package_name.get_size();
       name_index++) {
    Bool at_end = name_index == package_name.get_size();
    Bool at_type_access = !at_end && name_index + 1 < package_name.get_size() &&
                          package_name[name_index] == ':' &&
                          package_name[name_index + 1] == ':';
    if (!at_end && !at_type_access) {
      continue;
    }

    View::Bytes segment =
        package_name.slice(segment_start, name_index - segment_start);
    if (segment.is_empty()) {
      return False;
    }

    append_package_segment(output, segment);
    segment_count++;
    segment_start = name_index + (at_type_access ? 2 : 1);
    if (at_type_access) {
      name_index++;
    }
  }

  if (segment_count < 2) {
    output.clear();
    return False;
  }

  output.concat("/package.ttx"_view);
  return output.get_size() <= Path::max_size;
}

static auto package_error_message(
    View::Bytes package_name,
    View::Bytes package_path,
    Managed::Bytes& output) -> void {
  output.concat("Import could not find valid `"_view);
  output.concat(package_path);
  output.concat("` for package "_view);
  output.concat(package_name);
  output.concat(
      ".\nMake sure the Package is at the expected location or a Package that "
      "declares `@package_name = "_view);
  output.concat(package_name);
  output.concat("` is explicitly loaded."_view);
}

static auto stack_contains(
    const Dynamic::Vector<Dynamic::Bytes>& resolving,
    View::Bytes key) -> Bool {
  for (Count i = 0; i < resolving.get_size(); i++) {
    if (resolving[i] == key) {
      return True;
    }
  }

  return False;
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

static auto has_backslash(View::Bytes path) -> Bool {
  for (Count byte_index = 0; byte_index < path.get_size(); byte_index++) {
    if (path[byte_index] == '\\') {
      return True;
    }
  }

  return False;
}

static auto stack_remove(
    Dynamic::Vector<Dynamic::Bytes>& resolving,
    View::Bytes key) -> void {
  for (Count stack_index = resolving.get_size(); stack_index > 0;
       stack_index--) {
    if (resolving[stack_index - 1] == key) {
      resolving.remove(stack_index - 1);
      return;
    }
  }
}

static auto is_package_source(const Boot::Envelope& source) -> Bool {
  return source.get_isa() == Package::VirtualMachine::get_name();
}

static auto package_name_from_type(const Ttx::Type& type) -> View::Bytes {
  const Ttx::Type::Member* package_name = type.find_member("package_name"_view);
  if (package_name == nullptr || package_name->get_type() == nullptr) {
    return View::Bytes();
  }

  return package_name->get_type()->get_name();
}

static auto is_package_private_path(View::Bytes source_path) -> Bool {
  // Standard package implementation files are internal to Puffer resolution.
  // External source imports standard packages by name, then resolves exported
  // symbols through their package root.
  return starts_with(source_path, standard_source_root);
}

auto Resolver::Context::adopt_record(Dynamic::Object<Source::Record> record)
    -> Dynamic::Object<Source::Record> {
  Source::Record& adopted = *record;

  auto* existing = record_handles.find(&adopted);
  if (existing != nullptr) {
    return existing->value;
  }

  return record_handles.insert(&adopted, record)->value;
}

auto Resolver::Context::persist_errors(
    View::Vector<Ttx::Lexical::Errors::Error> source) -> void {
  for (Count i = 0; i < source.get_size(); i++) {
    persist_error(source[i]);
  }
}

auto Resolver::Context::persist_error(const Ttx::Lexical::Errors::Error& error)
    -> void {
  Ttx::Lexical::Source source(
      persist(error.get_source_path()), error.get_source());
  View::Bytes message = persist(error.get_message());
  View::Bytes hint = persist(error.get_hint());

  const Ttx::Lexical::Token* start = error.get_start_token();
  const Ttx::Lexical::Token* end = error.get_end_token();
  if (start == nullptr) {
    errors.insert(source, message, hint);
    return;
  }

  if (end == nullptr) {
    errors.insert(persist(*start), source, message, hint);
    return;
  }

  errors.insert_range(persist(*start), persist(*end), source, message, hint);
}

auto Resolver::Context::persist(View::Bytes bytes) -> View::Bytes {
  if (bytes.is_empty()) {
    return View::Bytes();
  }

  Bits_8* copy = error_arena.allocate(bytes.get_size());
  Data::copy(copy, bytes.get_data(), bytes.get_size());
  return View::Bytes(copy, bytes.get_size());
}

auto Resolver::Context::persist(const Ttx::Lexical::Token& token)
    -> const Ttx::Lexical::Token& {
  return error_arena.construct<Ttx::Lexical::Token>(
      persist(token.get_text()), token.get_class(), token.get_line(),
      token.get_column());
}

auto Resolver::resolve(View::Bytes key) -> Source::Record* {
  Source::Record* record = sources.find(key);
  if (record == nullptr || record->is_private()) {
    return nullptr;
  }

  return record;
}

auto Resolver::load_source(Context& context, View::Bytes source_path)
    -> Source::Record* {
  if (has_backslash(source_path)) {
    context.persist_errors(Ttx::Lexical::Errors::Error(
        source_path, View::Bytes(),
        "File paths must use `/` separators."_view));
    return nullptr;
  }

  Path normalized_path;
  if (!normalized_path.set(source_path) || normalized_path.is_rooted()) {
    context.persist_errors(Ttx::Lexical::Errors::Error(
        source_path, View::Bytes(),
        "Source path is outside the source tree."_view));
    return nullptr;
  }

  Source::Record* cached = sources.find(normalized_path.get_view());
  if (cached != nullptr && !cached->is_private()) {
    return cached;
  }

  // The disk entry point has the same contract as memory loading: return a
  // valid record with its imports resolved, or leave no public cache entry.
  Dynamic::Bytes source_text;
  if (!read_source(normalized_path.get_view(), source_text)) {
    context.persist_errors(Ttx::Lexical::Errors::Error(
        normalized_path.get_view(), View::Bytes(),
        "Imported source file could not be read."_view));
    return nullptr;
  }

  Dynamic::Vector<Dynamic::Bytes> resolving;
  return load_source(
      context, normalized_path.get_view(), source_text.get_view(), False,
      resolving);
}

auto Resolver::load_source(
    Context& context,
    View::Bytes source_path,
    View::Bytes ttx_content) -> Source::Record* {
  if (has_backslash(source_path)) {
    context.persist_errors(Ttx::Lexical::Errors::Error(
        source_path, ttx_content,
        "File paths must use `/` separators."_view));
    return nullptr;
  }

  Path normalized_path;
  if (!normalized_path.set(source_path) || normalized_path.is_rooted()) {
    context.persist_errors(Ttx::Lexical::Errors::Error(
        source_path, ttx_content,
        "Source path is outside the source tree."_view));
    return nullptr;
  }

  Dynamic::Vector<Dynamic::Bytes> resolving;
  return load_source(
      context, normalized_path.get_view(), ttx_content, False, resolving);
}

auto Resolver::reset() -> void {
  sources.reset();
}

auto Resolver::load_source(
    Context& context,
    View::Bytes source_path,
    View::Bytes ttx_content,
    Bool private_source,
    Dynamic::Vector<Dynamic::Bytes>& resolving,
    const Dynamic::Set<View::Bytes>* blocked_sources) -> Source::Record* {
  Dynamic::Vector<Snapshot> snapshots;
  Source::Record* existing = sources.find(source_path);
  if (existing != nullptr) {
    // Replacing a source may invalidate every source that imports it. Keep the
    // old consumer source text long enough to re-evaluate those consumers after
    // this source publishes its new record.
    snapshot_consumers(*existing, snapshots);
    sources.remove(*existing);
  }

  Dynamic::Object<Source::Record> record_handle = context.adopt_record(
      Dynamic::Object<Source::Record>(
          source_path, ttx_content, private_source));
  Source::Record& record = *record_handle;
  Allocator::Arena cursor_arena;
  Ttx::Lexical::Cursor cursor(record.get_tokenizer(), cursor_arena);
  Boot::Envelope* boot = evaluate_boot(cursor);
  if (boot == nullptr || cursor.get_errors().has_errors()) {
    context.persist_errors(cursor.get_errors());
    return nullptr;
  }

  record.set_boot(*boot);
  View::Bytes resolving_key = record.get_source_path();
  if (stack_contains(resolving, resolving_key)) {
    cursor.error("Import cycle detected while resolving source graph."_view);
    context.persist_errors(cursor.get_errors());
    return nullptr;
  }

  // `resolving` is the active depth-first path, not cached state. Cycles are
  // detected before a record is published, which keeps the cache graph acyclic.
  resolving.insert(Dynamic::Bytes(resolving_key));
  Dynamic::Vector<Source::Record*> producers;
  Bool resolved = resolve_imports(
      context, cursor, record, producers, resolving, blocked_sources);
  stack_remove(resolving, resolving_key);
  if (!resolved || cursor.get_errors().has_errors()) {
    context.persist_errors(cursor.get_errors());
    return nullptr;
  }

  Ttx::Type* type = execute_body(cursor, *boot, producers.get_view());
  if (type == nullptr && cursor.get_errors().is_empty()) {
    cursor.error("Selected ISA did not produce a TTX type."_view);
  }

  View::Bytes import_name = record.get_source_path();
  if (type != nullptr && is_package_source(*boot)) {
    import_name = package_name_from_type(*type);
  }
  if (import_name.is_empty()) {
    cursor.error("Package source did not declare a package name."_view);
  }

  if (type == nullptr || cursor.get_errors().has_errors()) {
    context.persist_errors(cursor.get_errors());
    return nullptr;
  }

  record.publish(*type, import_name);
  existing = sources.find(record.get_import_name());
  if (existing != nullptr) {
    // If the new source claims an import name already in the cache, the old
    // record must leave with its consumer tree before this record can publish.
    snapshot_consumers(*existing, snapshots);
    sources.remove(record.get_import_name());
  }

  // Publishing is the commit point. From here the cache may answer `resolve`
  // for this path or package name, and dependency indexes can point at it.
  // Resolving a package, even if it's valid, before it's published results in a
  // page fault for the evaluating machine which means cycle detection has to
  // happen during cascading Boot since it would just infinitely Boot loop on
  // imports otherwise.
  sources.publish(record_handle);
  for (Count i = 0; i < producers.get_size(); i++) {
    sources.connect(record, *producers[i]);
  }

  if (!private_source) {
    update_consumers(context, snapshots);
  }

  return &record;
}

auto Resolver::load_import(
    Context& context,
    Ttx::Lexical::Cursor& cursor,
    Source::Record& owner,
    const Boot::Import& import,
    Bool private_source,
    Dynamic::Vector<Dynamic::Bytes>& resolving,
    const Dynamic::Set<View::Bytes>* blocked_sources) -> Source::Record* {
  if (import.is_package()) {
    // TODO: Package imports should resolve from injected package resources,
    // usually Puffer Buffers emitted by `ttx_package` dependencies. The resolver
    // should hydrate the package's top TTX type from that resource and should
    // not know whether the package was built from source, cache, or another
    // producer. The source-path fallback below is only the bootstrap path until
    // Puffer Buffers can serialize and deserialize TTX type graphs.
    View::Bytes package_name = import.get_source_name();
    Source::Record* cached = sources.find(package_name);
    if (cached != nullptr) {
      return cached;
    }

    Dynamic::Bytes package_path;
    if (!package_name_to_source_path(package_name, package_path)) {
      cursor.error(
          "Import package path does not name a known package root."_view);
      return nullptr;
    }

    if (stack_contains(resolving, package_path.get_view())) {
      cursor.error("Import cycle detected while resolving source graph."_view);
      return nullptr;
    }

    File package_file;
    if (!package_file.read(package_path.get_view())) {
      Managed::Bytes message(cursor.get_arena());
      package_error_message(package_name, package_path.get_view(), message);
      cursor.error(message);
      return nullptr;
    }

    load_source(
        context, package_path.get_view(), package_file.get_view(), False,
        resolving, blocked_sources);
    return sources.find(package_name);
  }

  // File imports are local to the importing source. Package private files are
  // only reachable while resolving a package or one of its private imports.
  Path target_path;
  if (has_backslash(import.get_source_name())) {
    cursor.error("File paths must use `/` separators."_view);
    return nullptr;
  }

  if (!target_path.set_relative(
          owner.get_source_path(), import.get_source_name()) ||
      target_path.is_rooted()) {
    cursor.error("Import source path is outside the source tree."_view);
    return nullptr;
  }

  Bool target_private = private_source || owner.is_private() ||
                        is_package_source(owner.get_boot());
  if (is_package_private_path(target_path.get_view()) && !target_private) {
    cursor.error("Package private source cannot be imported directly."_view);
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

  if (stack_contains(resolving, target_path.get_view())) {
    cursor.error("Import cycle detected while resolving source graph."_view);
    return nullptr;
  }

  Dynamic::Bytes source_text;
  if (!read_source(target_path.get_view(), source_text)) {
    context.persist_errors(Ttx::Lexical::Errors::Error(
        target_path.get_view(), View::Bytes(),
        "Imported source file could not be read."_view));
    return nullptr;
  }

  load_source(
      context, target_path.get_view(), source_text.get_view(), target_private,
      resolving, blocked_sources);
  return sources.find(target_path.get_view());
}

auto Resolver::evaluate_boot(Ttx::Lexical::Cursor& cursor) -> Boot::Envelope* {
  // Resolver calls Puffer Boot directly for complete source files and passes
  // the toolchain's body ISA registry so the preamble can validate the selected
  // ISA and import ISA operands.
  return Boot::VirtualMachine::evaluate(cursor, toolchain.get_isa_registry());
}

auto Resolver::execute_body(
    Ttx::Lexical::Cursor& cursor,
    const Boot::Envelope& boot,
    View::Vector<Source::Record*> producers) -> Ttx::Type* {
  // Body ISAs execute after Boot has selected the instruction set and imports
  // are satisfied. That gives ISAs like Package the correct staging point to
  // bind authored references to real Ttx::Type addresses instead of inventing a
  // temporary semantic tree.
  Tetrodotoxin::Isa::Context isa_context(cursor.get_arena());
  auto imports = boot.get_imports();
  for (Count i = 0; i < producers.get_size(); i++) {
    Ttx::Type* type = producers[i]->get_type();
    if (type != nullptr &&
        !isa_context.define_type(imports[i].get_local_name(), *type)) {
      cursor.error("Import type name is already defined."_view);
      return nullptr;
    }
  }

  const auto* body_isa = toolchain.get_isa_registry().find(boot.get_isa());
  if (body_isa == nullptr) {
    cursor.error("Selected ISA is not installed in this toolchain."_view);
    return nullptr;
  }

  return body_isa->get_evaluator()(cursor, isa_context);
}

auto Resolver::resolve_imports(
    Context& context,
    Ttx::Lexical::Cursor& cursor,
    Source::Record& record,
    Dynamic::Vector<Source::Record*>& producers,
    Dynamic::Vector<Dynamic::Bytes>& resolving,
    const Dynamic::Set<View::Bytes>* blocked_sources) -> Bool {
  Bool resolved = True;

  auto requested_imports = record.get_boot().get_imports();
  // Evaluate every requested import so a single load reports as much of the
  // source error surface as possible. Producers are connected only after the
  // whole import list succeeds.
  for (Count i = 0; i < requested_imports.get_size(); i++) {
    const Boot::Import& import = requested_imports[i];
    Source::Record* producer = load_import(
        context, cursor, record, import, record.is_private(), resolving,
        blocked_sources);
    if (producer == nullptr) {
      resolved = False;
      continue;
    }

    if (import.get_isa() != producer->get_boot().get_isa()) {
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

auto Resolver::update_consumers(
    Context& context,
    Dynamic::Vector<Snapshot>& snapshots) -> void {
  Dynamic::Set<View::Bytes> blocked_sources;
  // Consumers are replayed from their old source text after a dependency
  // changes. If one fails, later consumers should still be checked, but they
  // must treat that failed source as unavailable instead of accidentally using
  // a stale record from earlier in the update.
  for (Count i = 0; i < snapshots.get_size(); i++) {
    Dynamic::Vector<Dynamic::Bytes> resolving;
    if (load_source(
            context, snapshots[i].source_path.get_view(),
            snapshots[i].source_text.get_view(), False, resolving,
            &blocked_sources) == nullptr) {
      blocked_sources.insert(snapshots[i].source_path.get_view());
    }
  }
}

auto Resolver::snapshot_consumers(
    Source::Record& record,
    Dynamic::Vector<Snapshot>& snapshots) -> void {
  Dynamic::Vector<Source::Record*> consumers;
  sources.collect_consumers(record, consumers);
  // Cache removal releases cache ownership, so consumer update work is captured
  // as path and source text snapshots before the dependency tree is removed.
  for (Count i = 0; i < consumers.get_size(); i++) {
    Snapshot snapshot;
    snapshot.source_path = consumers[i]->get_source_path();
    snapshot.source_text = consumers[i]->get_content();
    snapshots.insert(snapshot);
  }
}

auto Resolver::read_source(View::Bytes source_path, Dynamic::Bytes& source_text)
    -> Bool {
  File source_file;
  if (!source_file.read(source_path)) {
    return False;
  }

  source_text = source_file.get_view();
  return True;
}
