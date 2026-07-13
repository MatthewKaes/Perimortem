// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/puffer/resolution/package/cache.hpp"

#include "perimortem/memory/managed/bytes.hpp"
#include "perimortem/memory/managed/vector.hpp"

#include "ttx/lexical/cursor.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Tetrodotoxin::Puffer;

static auto missing_package_message(
    View::Bytes package_name,
    Managed::Bytes& output) -> void {
  output.concat("Puffer Buffer dependency is not registered: "_view);
  output.concat(package_name);
}

static auto package_version_message(
    View::Bytes package_name,
    Managed::Bytes& output) -> void {
  output.concat("Puffer Buffer dependency version mismatch: "_view);
  output.concat(package_name);
}

auto Resolution::Package::Cache::register_buffer(
    Resolution::Context& context,
    View::Bytes buffer_path,
    View::Bytes content) -> Bool {
  Dynamic::Object<Resolution::Package::Buffer> buffer(content);
  Allocator::Arena manifest_arena;
  Tetrodotoxin::Archiver::Manifest manifest =
      buffer->read_manifest(manifest_arena);
  View::Bytes package_name = manifest.get_name();
  if (package_name.is_empty()) {
    context.persist_errors(
        Ttx::Lexical::Errors::Error(
            buffer_path, content,
            "Puffer Buffer is not a package snapshot."_view));
    return False;
  }

  auto* existing = buffers.find(package_name);
  if (existing != nullptr) {
    Tetrodotoxin::Archiver::Manifest existing_manifest =
        existing->value->read_manifest(manifest_arena);
    if (existing_manifest.get_version() == manifest.get_version()) {
      return True;
    }

    context.persist_errors(
        Ttx::Lexical::Errors::Error(
            buffer_path, content,
            "Puffer Buffer package conflicts with an already registered "
            "version."_view));
    return False;
  }

  buffers.insert(package_name, buffer);
  return True;
}

auto Resolution::Package::Cache::load(
    Resolution::Context& context,
    Ttx::Lexical::Cursor& cursor,
    Source::Cache& sources,
    const Tetrodotoxin::Isa::Dialect& dialect,
    View::Bytes package_name,
    Dynamic::Set<View::Bytes>& active_includes) -> Source::Record* {
  Resolution::Package::Buffer* buffer = find_buffer(package_name);
  if (buffer == nullptr) {
    Managed::Bytes message(cursor.get_arena());
    missing_package_message(package_name, message);
    cursor.error(message);
    return nullptr;
  }

  Source::Record* cached = sources.find(package_name);
  if (cached != nullptr) {
    return cached;
  }

  if (active_includes.contains(package_name)) {
    cursor.error("Package buffer dependency cycle detected."_view);
    return nullptr;
  }

  Allocator::Arena manifest_arena;
  Tetrodotoxin::Archiver::Manifest manifest =
      buffer->read_manifest(manifest_arena);
  if (!manifest.is_valid()) {
    cursor.error("Puffer Buffer package imports could not be read."_view);
    return nullptr;
  }

  active_includes.insert(package_name);
  Dynamic::Vector<Source::Record*> producers;
  View::Vector<Tetrodotoxin::Archiver::Dependency> imports =
      manifest.get_imports();
  for (Count i = 0; i < imports.get_size(); i++) {
    Tetrodotoxin::Archiver::Dependency dependency = imports[i];
    View::Bytes dependency_name = dependency.get_source_name();
    Resolution::Package::Buffer* dependency_buffer =
        find_buffer(dependency_name);
    if (dependency_buffer == nullptr) {
      active_includes.remove(package_name);
      Managed::Bytes message(cursor.get_arena());
      missing_package_message(dependency_name, message);
      cursor.error(message);
      return nullptr;
    }

    Tetrodotoxin::Archiver::Manifest dependency_manifest =
        dependency_buffer->read_manifest(manifest_arena);
    if (dependency_manifest.get_version() != dependency.get_version()) {
      active_includes.remove(package_name);
      Managed::Bytes message(cursor.get_arena());
      package_version_message(dependency_name, message);
      cursor.error(message);
      return nullptr;
    }

    Source::Record* producer = load(
        context, cursor, sources, dialect, dependency_name, active_includes);
    if (producer == nullptr) {
      active_includes.remove(package_name);
      return nullptr;
    }

    producers.insert(producer);
  }

  Dynamic::Set<Source::Record*> referenced_records;
  Dynamic::Vector<Tetrodotoxin::Archiver::Reference> package_references;
  auto add_reference = [&](Source::Record& record) -> void {
    if (referenced_records.insert(&record)) {
      const auto* package = find(record.get_source_path());
      if (package != nullptr) {
        package_references.insert(Tetrodotoxin::Archiver::Reference(*package));
      }
    }
  };
  for (Count i = 0; i < producers.get_size(); i++) {
    add_reference(*producers[i]);
    sources.visit_producers(*producers[i], add_reference);
  }

  Dynamic::Object<Source::Record> record_handle(manifest.get_name());
  Source::Record& record = *record_handle;

  Tetrodotoxin::Archiver::Package* package = buffer->read_package(
      record.get_arena(), manifest, package_references.get_view());
  if (package == nullptr || !package->is_valid()) {
    active_includes.remove(package_name);
    cursor.error("Puffer Buffer package could not be restored."_view);
    return nullptr;
  }

  auto linkages = package->get_linkages();
  for (Count i = 0; i < linkages.get_size(); i++) {
    if (!record.get_implementation().define(linkages[i])) {
      active_includes.remove(package_name);
      cursor.error("Puffer Buffer linkage could not be restored."_view);
      return nullptr;
    }
  }

  Managed::Vector<Isa::Boot::Import> boot_imports(record.get_arena());
  View::Vector<Tetrodotoxin::Archiver::Dependency> package_imports =
      package->get_manifest().get_imports();
  boot_imports.reset(package_imports.get_size());
  for (Count i = 0; i < package_imports.get_size(); i++) {
    boot_imports.insert(
        Isa::Boot::Import::from_package(
            package_imports[i].get_local_name(),
            package_imports[i].get_source_name()));
  }

  if (!record.complete(dialect, boot_imports.get_view(), package->get_type())) {
    active_includes.remove(package_name);
    cursor.error("Puffer Buffer package record could not be completed."_view);
    return nullptr;
  }

  auto* restored_entry = restored.insert(
      package->get_manifest().get_name(), Restored(record_handle, *package));
  if (!sources.publish(restored_entry->value.get_record())) {
    active_includes.remove(package_name);
    cursor.error("Puffer Buffer package record could not be published."_view);
    return nullptr;
  }

  for (Count i = 0; i < producers.get_size(); i++) {
    sources.connect(record, *producers[i]);
  }

  active_includes.remove(package_name);
  return &record;
}

auto Resolution::Package::Cache::find_buffer(View::Bytes package_name)
    -> Resolution::Package::Buffer* {
  auto* entry = buffers.find(package_name);
  return entry == nullptr ? nullptr : &*entry->value;
}

auto Resolution::Package::Cache::find(View::Bytes package_name) const
    -> const Tetrodotoxin::Archiver::Package* {
  const auto* entry = restored.find(package_name);
  return entry == nullptr ? nullptr : &entry->value.get_package();
}
