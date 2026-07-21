// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/archiver/writer.hpp"

#include "perimortem/core/static/vector.hpp"
#include "perimortem/core/writer/binary.hpp"

#include "perimortem/memory/managed/bytes.hpp"
#include "perimortem/memory/managed/map.hpp"
#include "perimortem/memory/managed/vector.hpp"

#include "perimortem/serialization/stream/binary.hpp"

#include "tetrodotoxin/archiver/format.hpp"
#include "tetrodotoxin/model/namespace.hpp"
#include "ttx/concept/invalid.hpp"
#include "ttx/model/alias.hpp"
#include "ttx/model/exports.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Perimortem::Serialization;
using namespace Tetrodotoxin::Archiver;
using namespace Ttx::Concept;

using BinaryStream = Stream::Binary<Data::ByteOrder::Little, Managed::Bytes>;
using PatchWriter = Perimortem::Core::Writer::Binary<Data::ByteOrder::Little>;
using StringIndex = Managed::Map<View::Bytes, Count>;

class EncodedReference {
 public:
  Format::ReferenceCode code = Format::ReferenceCode::Local;
  Count dependency = Count(-1);
  Count definition = Count(-1);
};

class IndexedReference {
 public:
  EncodedReference reference;
  Bool ambiguous = False;
};

using ReferenceIndex = Managed::Map<const Ttx::Model::Alias*, EncodedReference>;
using ExternalIndex = Managed::Map<const Abstract*, IndexedReference>;

static auto write_size(BinaryStream& writer, Unsigned_64 value) -> void {
  do {
    Unsigned_8 byte = Unsigned_8(value & 0x7f);
    value >>= 7;
    if (value != 0) {
      byte |= 0x80;
    }

    writer << byte;
  } while (value != 0);
}

static auto write_bytes(BinaryStream& writer, View::Bytes value) -> void {
  write_size(writer, value.get_size());
  writer << value;
}

static auto find_string(const StringIndex& index, View::Bytes value) -> Count {
  const StringIndex::Entry* selected = index.find(value);
  return selected == nullptr ? Count(-1) : selected->value;
}

static auto add_string(
    Managed::Vector<View::Bytes>& strings,
    StringIndex& index,
    View::Bytes value) -> Count {
  Count existing = find_string(index, value);
  if (existing != Count(-1)) {
    return existing;
  }

  Count inserted = strings.get_size();
  strings.insert(value);
  index.insert(value, inserted);
  return inserted;
}

static auto add_external_reference(
    ExternalIndex& references,
    const Abstract& target,
    const EncodedReference& encoded) -> void {
  ExternalIndex::Entry* existing = references.find(&target);
  if (existing != nullptr) {
    existing->value.ambiguous = True;
    return;
  }

  IndexedReference indexed;
  indexed.reference = encoded;
  references.insert(&target, indexed);
}

static auto index_external_references(
    ExternalIndex& references,
    View::Vector<Ttx::Concept::Reference<Tetrodotoxin::Model::Package>>
        dependencies) -> Bool {
  for (Count i = 0; i < dependencies.get_size(); i++) {
    const Tetrodotoxin::Model::Package& dependency = dependencies[i].get();
    EncodedReference package_reference;
    package_reference.code = Format::ReferenceCode::Dependency;
    package_reference.dependency = i;
    add_external_reference(references, dependency, package_reference);

    for (Count definition = 0; definition < dependency.get_definition_count();
         definition++) {
      const Abstract& target = dependency.get_definition(definition);
      if (target.is<Invalid>() ||
          dependency.get_definition_id(target) != definition) {
        return False;
      }

      EncodedReference definition_reference;
      definition_reference.code = Format::ReferenceCode::DependencyDefinition;
      definition_reference.dependency = i;
      definition_reference.definition = definition;
      add_external_reference(references, target, definition_reference);
    }
  }

  return True;
}

static auto validate_record(
    const Abstract& record,
    const Tetrodotoxin::Model::Package& package) -> Bool {
  Bool supported = record.is<Tetrodotoxin::Model::Namespace>() ||
                   record.is<Ttx::Model::Alias>();
  if (!supported || record.get_name().is_empty()) {
    return False;
  }

  if (record.is<Tetrodotoxin::Model::Namespace>()) {
    const auto& namespace_object =
        record.assume<Tetrodotoxin::Model::Namespace>();
    for (Count i = 0; i < namespace_object.get_export_count(); i++) {
      if (package.get_definition_id(namespace_object.get_export(i)) ==
          Count(-1)) {
        return False;
      }
    }

    return True;
  }

  return True;
}

static auto resolve_reference(
    const Ttx::Model::Alias& alias,
    const ExternalIndex& external_references,
    const Tetrodotoxin::Model::Package& package,
    EncodedReference& result) -> Bool {
  const Abstract& target = alias.resolve();
  const ExternalIndex::Entry* external = external_references.find(&target);
  if (external != nullptr && !external->value.ambiguous) {
    result = external->value.reference;
    return True;
  }
  if (external != nullptr || !target.is<Tetrodotoxin::Model::Namespace>()) {
    return False;
  }

  Count local = package.get_definition_id(target);
  if (local == Count(-1)) {
    return False;
  }

  result.code = Format::ReferenceCode::Local;
  result.definition = local;
  return True;
}

static auto collect_documentation(
    const Documentation& documentation,
    Managed::Vector<View::Bytes>& strings,
    StringIndex& index) -> Bool {
  if (documentation.line_count() > Format::max_count) {
    return False;
  }

  for (Count i = 0; i < documentation.line_count(); i++) {
    add_string(strings, index, documentation.get_line(i));
  }

  return True;
}

static auto write_documentation(
    BinaryStream& writer,
    const Documentation& documentation,
    const StringIndex& strings) -> Bool {
  write_size(writer, documentation.line_count());
  for (Count i = 0; i < documentation.line_count(); i++) {
    Count string_index = find_string(strings, documentation.get_line(i));
    if (string_index == Count(-1)) {
      return False;
    }

    write_size(writer, string_index);
  }

  return True;
}

static auto write_reference(
    BinaryStream& writer,
    const EncodedReference& reference) -> void {
  writer << Unsigned_8(reference.code);
  if (reference.code == Format::ReferenceCode::Local) {
    write_size(writer, reference.definition);
    return;
  }

  write_size(writer, reference.dependency);
  if (reference.code == Format::ReferenceCode::DependencyDefinition) {
    write_size(writer, reference.definition);
  }
}

static auto collect_reference(
    ReferenceIndex& references,
    const Abstract& definition,
    const ExternalIndex& external_references,
    const Tetrodotoxin::Model::Package& package) -> Bool {
  if (!definition.is<Ttx::Model::Alias>()) {
    return True;
  }

  const Ttx::Model::Alias& alias = definition.assume<Ttx::Model::Alias>();
  EncodedReference reference;
  if (!resolve_reference(alias, external_references, package, reference)) {
    return False;
  }

  references.insert(&alias, reference);
  return True;
}

auto Tetrodotoxin::Archiver::Writer::write(
    Allocator::Arena& arena,
    const Manifest& manifest,
    const Tetrodotoxin::Model::Package& package,
    View::Vector<Tetrodotoxin::Model::Terminal> terminals) -> View::Bytes {
  if (!manifest.is_valid(arena) || terminals.get_size() > Format::max_count) {
    return {};
  }

  View::Vector<Reference<Tetrodotoxin::Model::Package>> dependencies =
      package.get_dependencies();
  if (manifest.get_dependencies().get_size() != dependencies.get_size()) {
    return {};
  }

  Managed::Map<View::Bytes, Bool> terminal_paths(arena);
  for (Count i = 0; i < terminals.get_size(); i++) {
    if (!Tetrodotoxin::Model::Terminal::is_valid_path(
            terminals[i].get_path()) ||
        terminal_paths.find(terminals[i].get_path()) != nullptr) {
      return {};
    }
    terminal_paths.insert(terminals[i].get_path(), True);
  }

  if (package.get_export_count() > Format::max_count ||
      package.get_definition_count() > Format::max_count) {
    return {};
  }
  ExternalIndex external_references(arena);
  if (!index_external_references(external_references, dependencies)) {
    return {};
  }
  ReferenceIndex references(arena);
  for (Count i = 0; i < package.get_definition_count(); i++) {
    const Abstract& definition = package.get_definition(i);
    if (definition.is<Invalid>() ||
        package.get_definition_id(definition) != i ||
        !validate_record(definition, package) ||
        !collect_reference(
            references, definition, external_references, package)) {
      return {};
    }
  }

  Managed::Map<View::Bytes, Bool> export_names(arena);
  for (Count i = 0; i < package.get_export_count(); i++) {
    const Abstract& edge = package.get_export(i);
    if (edge.get_name().is_empty() ||
        package.get_definition_id(edge) == Count(-1) ||
        export_names.find(edge.get_name()) != nullptr) {
      return {};
    }
    export_names.insert(edge.get_name(), True);
  }

  Managed::Vector<View::Bytes> strings(arena);
  StringIndex string_index(arena);
  View::Vector<Dependency> manifest_dependencies = manifest.get_dependencies();

  Bool root_documentation =
      collect_documentation(package.get_documentation(), strings, string_index);
  if (!root_documentation) {
    return {};
  }

  for (Count i = 0; i < package.get_definition_count(); i++) {
    const Abstract& record = package.get_definition(i);
    add_string(strings, string_index, record.get_name());
    Bool documented = collect_documentation(
        record.get_documentation(), strings, string_index);
    if (!documented) {
      return {};
    }
  }

  for (Count i = 0; i < terminals.get_size(); i++) {
    add_string(strings, string_index, terminals[i].get_path());
  }
  if (strings.get_size() > Format::max_count) {
    return {};
  }

  Managed::Bytes output(arena);
  BinaryStream writer(output);
  writer << Format::magic << Format::format_version;
  for (Count i = 0; i < Format::directory_count; i++) {
    writer << Unsigned_64(0);
  }

  Static::Vector<Unsigned_64, Format::directory_count> offsets;
  offsets[Count(Format::Section::Manifest)] = output.get_size();
  write_bytes(writer, manifest.get_name());
  write_size(writer, manifest.get_version().get_major());
  write_size(writer, manifest.get_version().get_minor());
  write_size(writer, manifest_dependencies.get_size());
  for (Count i = 0; i < manifest_dependencies.get_size(); i++) {
    write_bytes(writer, manifest_dependencies[i].get_name());
    write_size(writer, manifest_dependencies[i].get_version().get_major());
    write_size(writer, manifest_dependencies[i].get_version().get_minor());
  }

  offsets[Count(Format::Section::Strings)] = output.get_size();
  write_size(writer, strings.get_size());
  for (Count i = 0; i < strings.get_size(); i++) {
    write_size(writer, strings[i].get_size());
    writer << strings[i];
  }

  offsets[Count(Format::Section::Graph)] = output.get_size();
  Bool wrote_root_documentation =
      write_documentation(writer, package.get_documentation(), string_index);
  if (!wrote_root_documentation) {
    return {};
  }

  write_size(writer, package.get_export_count());
  for (Count i = 0; i < package.get_export_count(); i++) {
    Count record_index = package.get_definition_id(package.get_export(i));
    if (record_index == Count(-1)) {
      return {};
    }

    write_size(writer, record_index);
  }

  write_size(writer, package.get_definition_count());
  for (Count i = 0; i < package.get_definition_count(); i++) {
    const Abstract& record = package.get_definition(i);
    if (record.is<Tetrodotoxin::Model::Namespace>()) {
      writer << Unsigned_8(Format::RecordCode::Namespace);
    } else if (record.is<Ttx::Model::Alias>()) {
      writer << Unsigned_8(Format::RecordCode::Alias);
    } else {
      return {};
    }

    write_size(writer, find_string(string_index, record.get_name()));
    Bool wrote_documentation =
        write_documentation(writer, record.get_documentation(), string_index);
    if (!wrote_documentation) {
      return {};
    }

    if (record.is<Tetrodotoxin::Model::Namespace>()) {
      const auto& namespace_object =
          record.assume<Tetrodotoxin::Model::Namespace>();
      write_size(writer, namespace_object.get_export_count());
      for (Count k = 0; k < namespace_object.get_export_count(); k++) {
        Count child = package.get_definition_id(namespace_object.get_export(k));
        if (child == Count(-1)) {
          return {};
        }

        write_size(writer, child);
      }
      continue;
    }

    const auto* reference =
        references.find(&record.assume<Ttx::Model::Alias>());
    if (reference == nullptr) {
      return {};
    }
    write_reference(writer, reference->value);
  }

  offsets[Count(Format::Section::Terminals)] = output.get_size();
  write_size(writer, terminals.get_size());
  for (Count i = 0; i < terminals.get_size(); i++) {
    write_size(writer, find_string(string_index, terminals[i].get_path()));
    write_size(writer, terminals[i].get_content().get_size());
    writer << terminals[i].get_content();
  }
  offsets[Count(Format::Section::End)] = output.get_size();

  PatchWriter patch(output.get_access());
  for (Count i = 0; i < Format::directory_count; i++) {
    patch.set_pointer(Format::slot(Format::Section(i)));
    patch << offsets[i];
  }

  return output.get_view();
}
