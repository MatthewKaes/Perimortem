// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/resolution/resolver.hpp"

#include "perimortem/core/data.hpp"

#include "perimortem/system/file.hpp"
#include "perimortem/system/path.hpp"

#include "ttx/dialect/lingua_franca.hpp"
#include "ttx/lexical/cursor.hpp"
#include "ttx/lexical/tokenizer.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Perimortem::System;
using namespace Tetrodotoxin::Resolution;

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
  Count segment_start = 0;
  Count segment_count = 0;

  // Package names use symbol spelling at the source surface but package roots
  // are regular source-tree paths. Walk the Type segment by segment so
  // `Perimortem::Graphics` naturally becomes `perimortem/graphics/package.ttx`
  // without a root lookup table.
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
    Dynamic::Bytes& output) -> void {
  output = "Import could not find valid `"_view;
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
  for (Count stack_index = 0; stack_index < resolving.get_size();
       stack_index++) {
    if (resolving[stack_index] == key) {
      return True;
    }
  }

  return False;
}

static auto blocked_contains(
    const Dynamic::Vector<Dynamic::Bytes>* blocked_sources,
    View::Bytes key) -> Bool {
  if (blocked_sources == nullptr) {
    return False;
  }

  for (Count source_index = 0; source_index < blocked_sources->get_size();
       source_index++) {
    if ((*blocked_sources)[source_index] == key) {
      return True;
    }
  }

  return False;
}

static auto unresolved_import_message(
    View::Bytes import_name,
    Dynamic::Bytes& output) -> void {
  output = "Couldn't resolve import `"_view;
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

static auto is_package_source(const Ttx::Dialect::Source::Source& source)
    -> Bool {
  return source.get_dialect().get_name() == "Package"_view;
}

static auto is_package_private_path(View::Bytes source_path) -> Bool {
  // Engine ABI package files are implementation details. External source
  // imports packages by name, then resolves exported symbols through them.
  return starts_with(source_path, "perimortem/"_view);
}

// Temporary resolver stopgap. Everything in this namespace belongs in the
// Package dialect once that dialect owns package metadata parsing.
namespace Package {

static auto parse_symbol_path(Ttx::Lexical::Cursor& cursor) -> View::Bytes {
  const Ttx::Lexical::Token* first_segment = cursor.require(
      Ttx::Lexical::Class::Type::Type,
      "Expected package name to start with a Type name."_view);
  if (first_segment == nullptr) {
    return View::Bytes();
  }

  const Ttx::Lexical::Token* last_segment = first_segment;
  // The cursor has already tokenized the package file, so a package name can be
  // returned as one contiguous source view from the first segment through the
  // final segment instead of copying it into resolver-owned storage.
  while (cursor.matches(Ttx::Lexical::Class::Type::TypeAccessOp)) {
    cursor.consume();
    last_segment = cursor.require(
        Ttx::Lexical::Class::Type::Type,
        "Package name segments should all be Type names."_view);
    if (last_segment == nullptr) {
      return View::Bytes();
    }
  }

  auto start = first_segment->get_text();
  auto end = last_segment->get_text();
  return View::Bytes(
      start.get_data(), end.get_data() - start.get_data() + end.get_size());
}

static auto parse_package_name(Ttx::Lexical::Cursor& cursor) -> View::Bytes {
  // Package identity is metadata owned by the Package dialect. The source
  // envelope parser leaves the cursor at the dialect body, so this scan only
  // looks for the package-name attribute and ignores other package members.
  while (!cursor.matches(Ttx::Lexical::Class::Type::EndOfStream)) {
    const Ttx::Lexical::Token& token = cursor.current();
    if (token.get_class() != Ttx::Lexical::Class::Type::Attribute ||
        token.get_text() != "@package_name"_view) {
      cursor.consume();
      continue;
    }

    cursor.consume();
    if (!cursor.require(
            Ttx::Lexical::Class::Type::Assign,
            "Expected `=` after @package_name."_view)) {
      return View::Bytes();
    }

    View::Bytes package_name = parse_symbol_path(cursor);
    if (package_name.is_empty()) {
      return View::Bytes();
    }

    cursor.require(
        Ttx::Lexical::Class::Type::EndStatement,
        "Expected `;` after package name."_view);
    return package_name;
  }

  cursor.error("Package source must declare `@package_name`."_view);
  return View::Bytes();
}

}  // namespace Package

auto Resolver::Context::reset() -> void {
  errors.clear();
  error_arena.reset();
}

auto Resolver::Context::add_error(const Ttx::Lexical::Error& error) -> void {
  // The resolver context is the diagnostic lifetime boundary for one request.
  // Parser errors point into tokenizers and records that may be destroyed when
  // a failed source is rejected, so copy every view a surfaced error needs.
  View::Bytes source_path = keep(error.get_source_path());
  View::Bytes source = keep(error.get_source());
  View::Bytes message = keep(error.get_message());
  View::Bytes hint = keep(error.get_hint());

  const Ttx::Lexical::Token* start_token = error.get_start_token();
  const Ttx::Lexical::Token* end_token = error.get_end_token();
  if (start_token == nullptr) {
    errors.insert(Ttx::Lexical::Error(source_path, source, message, hint));
    return;
  }

  Ttx::Lexical::Token& start = error_arena.construct<Ttx::Lexical::Token>(
      keep(start_token->get_text()), start_token->get_class(),
      start_token->get_line(), start_token->get_column());
  if (end_token == nullptr) {
    errors.insert(
        Ttx::Lexical::Error(start, source_path, source, message, hint));
    return;
  }

  Ttx::Lexical::Token& end = error_arena.construct<Ttx::Lexical::Token>(
      keep(end_token->get_text()), end_token->get_class(),
      end_token->get_line(), end_token->get_column());
  errors.insert(
      Ttx::Lexical::Error(start, end, source_path, source, message, hint));
}

auto Resolver::Context::keep(View::Bytes bytes) -> View::Bytes {
  if (bytes.is_empty()) {
    return View::Bytes();
  }

  Bits_8* copy = error_arena.allocate(bytes.get_size());
  Data::copy(copy, bytes.get_data(), bytes.get_size());
  return View::Bytes(copy, bytes.get_size());
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
    add_error(
        context, source_path, View::Bytes(),
        "File paths must use `/` separators."_view);
    return nullptr;
  }

  Path normalized_path;
  if (!normalized_path.set(source_path) || normalized_path.is_rooted()) {
    add_error(
        context, source_path, View::Bytes(),
        "Source path is outside the source tree."_view);
    return nullptr;
  }

  Source::Record* cached = sources.find(normalized_path.get_view());
  if (cached != nullptr && !cached->is_private()) {
    return cached;
  }

  // The disk entry point has the same contract as memory loading: return a
  // valid record with its imports resolved, or leave no public cache entry.
  Dynamic::Bytes source_text;
  if (!read_source(context, normalized_path.get_view(), source_text)) {
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
    add_error(
        context, source_path, ttx_content,
        "File paths must use `/` separators."_view);
    return nullptr;
  }

  Path normalized_path;
  if (!normalized_path.set(source_path) || normalized_path.is_rooted()) {
    add_error(
        context, source_path, ttx_content,
        "Source path is outside the source tree."_view);
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
    const Dynamic::Vector<Dynamic::Bytes>* blocked_sources) -> Source::Record* {
  Dynamic::Vector<Snapshot> snapshots;
  Source::Record* existing = sources.find(source_path);
  if (existing != nullptr) {
    // Replacing a source may invalidate every source that imports it. Keep the
    // old consumer source text long enough to re-evaluate those consumers after
    // this source publishes its new record.
    snapshot_consumers(*existing, snapshots);
    sources.remove(source_path);
  }

  Source::Record& record =
      Source::Record::create(source_path, ttx_content, private_source);
  if (!parse_source(context, record)) {
    Source::Record::destroy(record);
    return nullptr;
  }

  existing = sources.find(record.get_import_name());
  if (existing != nullptr) {
    // Package sources are keyed by both their file path and declared package
    // name. If the new file claims a package name already in the cache, the old
    // package must leave with its consumer tree before this record can publish.
    snapshot_consumers(*existing, snapshots);
    sources.remove(record.get_import_name());
  }

  View::Bytes resolving_key = record.get_import_name();
  if (stack_contains(resolving, resolving_key)) {
    add_error(
        context, record.get_source_path(), record.get_content(),
        "Import cycle detected while resolving source graph."_view);
    Source::Record::destroy(record);
    return nullptr;
  }

  // `resolving` is the active depth-first path, not cached state. Cycles are
  // detected before a record is published, which keeps the cache graph acyclic.
  resolving.insert(Dynamic::Bytes(resolving_key));
  Bool resolved = resolve_imports(context, record, resolving, blocked_sources);
  stack_remove(resolving, resolving_key);
  if (!resolved) {
    Source::Record::destroy(record);
    return nullptr;
  }

  if (!private_source) {
    update_consumers(context, snapshots);
  }

  return &record;
}

auto Resolver::load_import(
    Context& context,
    Source::Record& owner,
    const Ttx::Dialect::Source::Import& import,
    Bool private_source,
    Dynamic::Vector<Dynamic::Bytes>& resolving,
    const Dynamic::Vector<Dynamic::Bytes>* blocked_sources) -> Source::Record* {
  if (import.is_package()) {
    // Packages are addressed by public package name. Their internal source path
    // is derived only as a way to load the package root when it is not already
    // in the cache.
    View::Bytes package_name = import.get_source_name();
    Source::Record* cached = sources.find(package_name);
    if (cached != nullptr) {
      return cached;
    }

    Dynamic::Bytes package_path;
    if (!package_name_to_source_path(package_name, package_path)) {
      add_error(
          context, owner.get_source_path(), owner.get_content(),
          "Import package path does not name a known package root."_view);
      return nullptr;
    }

    if (stack_contains(resolving, package_name)) {
      add_error(
          context, owner.get_source_path(), owner.get_content(),
          "Import cycle detected while resolving source graph."_view);
      return nullptr;
    }

    File package_file;
    if (!package_file.read(package_path.get_view())) {
      Dynamic::Bytes message;
      package_error_message(package_name, package_path.get_view(), message);
      add_error(
          context, owner.get_source_path(), owner.get_content(),
          message.get_view());
      return nullptr;
    }

    load_source(
        context, package_path.get_view(), package_file.get_view(), False,
        resolving, blocked_sources);
    return sources.find(package_name);
  }

  // File imports are local to the importing source. Package-private files are
  // only reachable while resolving a package or one of its private imports.
  Path target_path;
  if (has_backslash(import.get_source_name())) {
    add_error(
        context, owner.get_source_path(), owner.get_content(),
        "File paths must use `/` separators."_view);
    return nullptr;
  }

  if (!target_path.set_relative(
          owner.get_source_path(), import.get_source_name()) ||
      target_path.is_rooted()) {
    add_error(
        context, owner.get_source_path(), owner.get_content(),
        "Import source path is outside the source tree."_view);
    return nullptr;
  }

  Bool target_private = private_source || owner.is_private() ||
                        is_package_source(owner.get_source());
  if (is_package_private_path(target_path.get_view()) && !target_private) {
    add_error(
        context, owner.get_source_path(), owner.get_content(),
        "Package private source cannot be imported directly."_view);
    return nullptr;
  }

  if (blocked_contains(blocked_sources, target_path.get_view())) {
    Dynamic::Bytes message;
    unresolved_import_message(target_path.get_view(), message);
    add_error(
        context, owner.get_source_path(), owner.get_content(),
        message.get_view());
    return nullptr;
  }

  Source::Record* cached = sources.find(target_path.get_view());
  if (cached != nullptr) {
    return cached;
  }

  if (stack_contains(resolving, target_path.get_view())) {
    add_error(
        context, owner.get_source_path(), owner.get_content(),
        "Import cycle detected while resolving source graph."_view);
    return nullptr;
  }

  Dynamic::Bytes source_text;
  if (!read_source(context, target_path.get_view(), source_text)) {
    return nullptr;
  }

  load_source(
      context, target_path.get_view(), source_text.get_view(), target_private,
      resolving, blocked_sources);
  return sources.find(target_path.get_view());
}

auto Resolver::parse_source(Context& context, Source::Record& record) -> Bool {
  // Resolution deliberately parses the Source dialect through the dialect
  // registry. That keeps the resolver consuming the same lingua franca path as
  // other tools instead of constructing source envelopes by hand.
  Ttx::Lexical::Tokenizer tokenizer(
      record.get_arena(), record.get_content(), record.get_source_path());
  Ttx::Lexical::Cursor cursor(tokenizer);
  const Ttx::Dialect::LinguaFranca source_dialect =
      Ttx::Dialect::LinguaFranca::find(
          Ttx::Dialect::Source::Source::get_name());
  if (source_dialect.get_parser() == nullptr) {
    add_error(
        context, record.get_source_path(), record.get_content(),
        "Source dialect is not registered."_view);
    return False;
  }

  void* parsed_source = source_dialect.get_parser()(cursor);
  auto* source = Data::cast<Ttx::Dialect::Source::Source>(parsed_source);
  View::Bytes import_name = record.get_source_path();
  if (source != nullptr && is_package_source(*source)) {
    // Package sources publish under their declared package name. Regular source
    // files publish under their normalized path.
    import_name = Package::parse_package_name(cursor);
  }

  auto parse_errors = cursor.get_errors();
  // Parse diagnostics are copied out before the record can be destroyed on
  // failure. The resolver is only a pass-through for these errors.
  for (Count error_index = 0; error_index < parse_errors.get_size();
       error_index++) {
    context.add_error(parse_errors[error_index]);
  }

  if (source == nullptr || !parse_errors.is_empty() || import_name.is_empty()) {
    return False;
  }

  record.publish(*source, import_name);
  return True;
}

auto Resolver::resolve_imports(
    Context& context,
    Source::Record& record,
    Dynamic::Vector<Dynamic::Bytes>& resolving,
    const Dynamic::Vector<Dynamic::Bytes>* blocked_sources) -> Bool {
  Bool resolved = True;
  Dynamic::Vector<Source::Record*> producers;

  auto requested_imports = record.get_source().get_imports();
  // Evaluate every requested import so a single load reports as much of the
  // source-tree error surface as possible. Producers are connected only after
  // the whole import list succeeds.
  for (Count import_index = 0; import_index < requested_imports.get_size();
       import_index++) {
    const Ttx::Dialect::Source::Import& import =
        requested_imports[import_index];
    Source::Record* producer = load_import(
        context, record, import, record.is_private(), resolving,
        blocked_sources);
    if (producer == nullptr) {
      resolved = False;
      continue;
    }

    if (!(import.get_dialect() == producer->get_source().get_dialect())) {
      add_error(
          context, record.get_source_path(), record.get_content(),
          "Imported source dialect does not match the requested dialect."_view);
      resolved = False;
      continue;
    }

    producers.insert(producer);
  }

  if (!resolved) {
    return False;
  }

  // Publishing is the commit point: from here the cache may answer `resolve`
  // for this path/package name, and the dependency indexes can point at it.
  sources.publish(record);

  for (Count producer_index = 0; producer_index < producers.get_size();
       producer_index++) {
    sources.connect(record, *producers[producer_index]);
  }

  return True;
}

auto Resolver::update_consumers(
    Context& context,
    Dynamic::Vector<Snapshot>& snapshots) -> void {
  Dynamic::Vector<Dynamic::Bytes> blocked_sources;
  // Consumers are replayed from their old source text after a dependency
  // changes. If one fails, later consumers should still be checked, but they
  // must treat that failed source as unavailable instead of accidentally using
  // a stale record from earlier in the update.
  for (Count snapshot_index = 0; snapshot_index < snapshots.get_size();
       snapshot_index++) {
    Dynamic::Vector<Dynamic::Bytes> resolving;
    if (load_source(
            context, snapshots[snapshot_index].source_path.get_view(),
            snapshots[snapshot_index].source_text.get_view(), False, resolving,
            &blocked_sources) == nullptr) {
      blocked_sources.insert(snapshots[snapshot_index].source_path);
    }
  }
}

auto Resolver::snapshot_consumers(
    Source::Record& record,
    Dynamic::Vector<Snapshot>& snapshots) -> void {
  Dynamic::Vector<Source::Record*> consumers;
  sources.collect_transitive_consumers(record, consumers);
  // Cache removal destroys records, so consumer update work is captured as
  // source-path/source-text snapshots before the dependency tree is removed.
  for (Count consumer_index = 0; consumer_index < consumers.get_size();
       consumer_index++) {
    Snapshot snapshot;
    snapshot.source_path = consumers[consumer_index]->get_source_path();
    snapshot.source_text = consumers[consumer_index]->get_content();
    snapshots.insert(snapshot);
  }
}

auto Resolver::read_source(
    Context& context,
    View::Bytes source_path,
    Dynamic::Bytes& source_text) -> Bool {
  File source_file;
  if (!source_file.read(source_path)) {
    add_error(
        context, source_path, View::Bytes(),
        "Imported source file could not be read."_view);
    return False;
  }

  source_text = source_file.get_view();
  return True;
}

auto Resolver::add_error(
    Context& context,
    View::Bytes source_path,
    View::Bytes source_text,
    View::Bytes message) -> void {
  context.add_error(Ttx::Lexical::Error(source_path, source_text, message));
}
