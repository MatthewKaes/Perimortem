// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/puffer/resolution/package/repository.hpp"

#include "perimortem/memory/managed/vector.hpp"

#include "perimortem/system/file.hpp"

#include "tetrodotoxin/archiver/reader.hpp"
#include "ttx/concept/invalid.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Perimortem::System;
using namespace Tetrodotoxin::Archiver;
using namespace Tetrodotoxin::Puffer::Resolution::Package;
using namespace Ttx::Concept;

Repository::Record::Record(View::Bytes content, View::Bytes diagnostic_path)
    : content(arena, content),
      diagnostic_path(arena.proxy(diagnostic_path)),
      manifest(nullptr),
      result(&Invalid::get_invalid()) {
  manifest = Reader(this->content).read_manifest(arena);
}

auto Repository::Record::is_valid() const -> Bool {
  return manifest != nullptr;
}

auto Repository::Record::get_content() const -> View::Bytes {
  return content;
}

auto Repository::Record::get_manifest() const -> const Manifest& {
  return *manifest;
}

auto Repository::Record::get_result() const -> const Abstract& {
  return *result;
}

auto Repository::Record::publish(const Model::Package& package) -> void {
  result = &package;
}

auto Repository::Record::get_arena() -> Allocator::Arena& {
  return arena;
}

auto Repository::register_file(View::Bytes path) -> Bool {
  Dynamic::Bytes content = File::read(path);
  if (content.is_empty()) {
    return False;
  }

  return register_buffer(content, path);
}

auto Repository::register_buffer(
    View::Bytes content,
    View::Bytes diagnostic_path) -> Bool {
  Dynamic::Object<Record> candidate(content, diagnostic_path);
  if (!candidate->is_valid()) {
    return False;
  }

  const Manifest& manifest = candidate->get_manifest();
  Record* existing = find(manifest.get_name(), manifest.get_version());
  if (existing != nullptr) {
    return existing->get_content() == content;
  }

  Record* record = &*candidate;
  records.insert(candidate);
  records_by_key.insert(
      Key(manifest.get_name(), manifest.get_version()), record);
  return True;
}

auto Repository::find_manifest(View::Bytes name, Version version) const
    -> const Manifest* {
  const Record* record = find(name, version);
  return record == nullptr ? nullptr : &record->get_manifest();
}

auto Repository::resolve(View::Bytes name, Version version) -> const Abstract& {
  Record* record = find(name, version);
  if (record == nullptr) {
    return Invalid::get_invalid();
  }

  Dynamic::Set<Record*> active;
  return resolve(*record, active);
}

auto Repository::resolve(
    Model::Environment& environment,
    const Model::Dialect& root_dialect,
    View::Bytes local_name,
    View::Bytes package_name,
    Version version) -> Bool {
  const Abstract& selected = resolve(package_name, version);
  if (!selected.is<Model::Package>()) {
    return False;
  }

  const Model::Package& package = selected.assume<Model::Package>();
  return environment.resolve(
      root_dialect, local_name, package_name, version, package,
      package.get_documentation());
}

auto Repository::find(View::Bytes name, Version version) -> Record* {
  return const_cast<Record*>(
      static_cast<const Repository*>(this)->find(name, version));
}

auto Repository::find(View::Bytes name, Version version) const
    -> const Record* {
  const auto* selected = records_by_key.find(Key(name, version));
  return selected == nullptr ? nullptr : selected->value;
}

auto Repository::resolve(Record& record, Dynamic::Set<Record*>& active)
    -> const Abstract& {
  if (record.get_result().is<Model::Package>()) {
    return record.get_result();
  }
  if (active.contains(&record)) {
    return Invalid::get_invalid();
  }

  active.insert(&record);
  Allocator::Arena& arena = record.get_arena();
  Managed::Vector<Reference<Model::Package>> dependencies(arena);
  View::Vector<Dependency> requested = record.get_manifest().get_dependencies();
  dependencies.reset(requested.get_size());
  for (Count i = 0; i < requested.get_size(); i++) {
    Record* dependency_record =
        find(requested[i].get_name(), requested[i].get_version());
    if (dependency_record == nullptr) {
      active.remove(&record);
      return Invalid::get_invalid();
    }

    const Abstract& dependency = resolve(*dependency_record, active);
    if (!dependency.is<Model::Package>()) {
      active.remove(&record);
      return Invalid::get_invalid();
    }

    dependencies.insert(
        Reference<Model::Package>(dependency.assume<Model::Package>()));
  }

  const Abstract& restored =
      Reader(record.get_content())
          .read_package(arena, record.get_manifest(), dependencies.get_view());
  active.remove(&record);
  if (!restored.is<Model::Package>()) {
    return Invalid::get_invalid();
  }

  record.publish(restored.assume<Model::Package>());
  return record.get_result();
}
