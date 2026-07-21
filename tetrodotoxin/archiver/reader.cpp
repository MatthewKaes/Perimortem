// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/archiver/reader.hpp"

#include "perimortem/core/static/vector.hpp"
#include "perimortem/core/reader/binary.hpp"

#include "perimortem/memory/managed/map.hpp"
#include "perimortem/memory/managed/vector.hpp"

#include "tetrodotoxin/archiver/format.hpp"
#include "tetrodotoxin/model/namespace.hpp"
#include "tetrodotoxin/model/packages/precompiled.hpp"
#include "tetrodotoxin/model/terminal.hpp"
#include "ttx/concept/invalid.hpp"
#include "ttx/model/alias.hpp"
#include "ttx/model/documentations/block.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Perimortem::System;
using namespace Tetrodotoxin::Archiver;
using namespace Ttx::Concept;

using BinaryReader = Perimortem::Core::Reader::Binary<Data::ByteOrder::Little>;

class DecodedRecord {
 public:
  Format::RecordCode code = Format::RecordCode::Namespace;
  View::Bytes name;
  View::Vector<View::Bytes> documentation;
  View::Vector<Count> exports;
  Format::ReferenceCode reference = Format::ReferenceCode::Local;
  Count target = Count(-1);
  Count dependency = Count(-1);
};

class ScopedName {
 public:
  constexpr ScopedName(Count scope, View::Bytes name)
      : scope(scope), name(name) {}

  constexpr auto operator==(const ScopedName& rhs) const -> Bool {
    return scope == rhs.scope && name == rhs.name;
  }

  constexpr auto hash() const -> Unsigned_64 {
    return Hash(name).Rehash(scope);
  }

 private:
  Count scope;
  View::Bytes name;
};

static auto read_size(BinaryReader& reader, Unsigned_64& value) -> Bool {
  value = 0;
  for (Count byte_index = 0; byte_index < 10; byte_index++) {
    Unsigned_8 byte = reader.read_unsigned_8();
    if (reader.get_location() == Count(-1)) {
      return False;
    }

    Unsigned_8 payload = byte & 0x7f;
    if (byte_index == 9 && (payload > 1 || (byte & 0x80) != 0)) {
      return False;
    }

    value |= Unsigned_64(payload) << (byte_index * 7);
    if ((byte & 0x80) == 0) {
      return byte_index == 0 || payload != 0;
    }
  }

  return False;
}

static auto read_count(BinaryReader& reader, Count& value) -> Bool {
  Unsigned_64 encoded = 0;
  Bool read = read_size(reader, encoded);
  if (!read || encoded > Format::max_count) {
    return False;
  }

  value = Count(encoded);
  return True;
}

static auto read_bytes(
    Allocator::Arena& arena,
    BinaryReader& reader,
    View::Bytes& value) -> Bool {
  Unsigned_64 encoded_size = 0;
  if (!read_size(reader, encoded_size) ||
      encoded_size > reader.get_size() - reader.get_location()) {
    return False;
  }

  View::Bytes decoded = reader.read_bytes(Count(encoded_size));
  if (reader.get_location() == Count(-1)) {
    return False;
  }

  value = arena.proxy(decoded);
  return True;
}

static auto read_sections(
    View::Bytes source,
    Static::Vector<View::Bytes, Format::section_count>& sections) -> Bool {
  if (source.get_size() < Format::header_size) {
    return False;
  }

  BinaryReader reader(source);
  View::Bytes magic = reader.read_bytes(Format::magic.get_size());
  Unsigned_32 version = reader.read_unsigned_32();
  if (magic != Format::magic || version != Format::format_version) {
    return False;
  }

  Static::Vector<Unsigned_64, Format::directory_count> offsets;
  for (Count i = 0; i < Format::directory_count; i++) {
    offsets[i] = reader.read_unsigned_64();
  }
  if (reader.get_location() == Count(-1) || offsets[0] != Format::header_size ||
      offsets[Format::section_count] != source.get_size()) {
    return False;
  }

  for (Count i = 0; i < Format::section_count; i++) {
    if (offsets[i] >= offsets[i + 1] || offsets[i + 1] > source.get_size()) {
      return False;
    }

    sections[i] =
        source.slice(Count(offsets[i]), Count(offsets[i + 1] - offsets[i]));
  }

  return True;
}

static auto read_strings(
    Allocator::Arena& arena,
    View::Bytes source,
    Managed::Vector<View::Bytes>& strings) -> Bool {
  BinaryReader reader(source);
  Count string_count = 0;
  if (!read_count(reader, string_count) ||
      string_count > reader.get_size() - reader.get_location()) {
    return False;
  }

  strings.reset(string_count);
  Managed::Map<View::Bytes, Bool> unique(arena);
  unique.ensure_capacity(string_count);
  for (Count i = 0; i < string_count; i++) {
    Unsigned_64 encoded_size = 0;
    Bool read_length = read_size(reader, encoded_size);
    if (!read_length ||
        encoded_size > reader.get_size() - reader.get_location()) {
      return False;
    }

    View::Bytes value = reader.read_bytes(Count(encoded_size));
    if (reader.get_location() == Count(-1)) {
      return False;
    }
    if (unique.find(value) != nullptr) {
      return False;
    }

    View::Bytes owned = arena.proxy(value);
    strings.insert(owned);
    unique.insert(owned, True);
  }

  return reader.get_location() == reader.get_size();
}

static auto read_string(
    BinaryReader& reader,
    View::Vector<View::Bytes> strings,
    View::Bytes& value) -> Bool {
  Count index = 0;
  if (!read_count(reader, index) || index >= strings.get_size()) {
    return False;
  }

  value = strings[index];
  return True;
}

static auto read_documentation(
    Allocator::Arena& arena,
    BinaryReader& reader,
    View::Vector<View::Bytes> strings,
    View::Vector<View::Bytes>& documentation) -> Bool {
  Count line_count = 0;
  if (!read_count(reader, line_count) ||
      line_count > reader.get_size() - reader.get_location()) {
    return False;
  }

  Managed::Vector<View::Bytes> lines(arena);
  lines.reset(line_count);
  for (Count i = 0; i < line_count; i++) {
    View::Bytes line;
    if (!read_string(reader, strings, line)) {
      return False;
    }

    lines.insert(line);
  }

  documentation = lines.get_view();
  return True;
}

static auto read_indices(
    Allocator::Arena& arena,
    BinaryReader& reader,
    View::Vector<Count>& indices) -> Bool {
  Count count = 0;
  if (!read_count(reader, count) ||
      count > reader.get_size() - reader.get_location()) {
    return False;
  }

  Managed::Vector<Count> values(arena);
  values.reset(count);
  for (Count i = 0; i < count; i++) {
    Count value = 0;
    if (!read_count(reader, value)) {
      return False;
    }

    values.insert(value);
  }

  indices = values.get_view();
  return True;
}

static auto read_manifest_section(Allocator::Arena& arena, View::Bytes source)
    -> const Manifest* {
  BinaryReader reader(source);
  View::Bytes name;
  Unsigned_64 major = 0;
  Unsigned_64 minor = 0;
  if (!read_bytes(arena, reader, name) || !read_size(reader, major) ||
      !read_size(reader, minor) || major > Unsigned_16(-1) ||
      minor > Unsigned_16(-1)) {
    return nullptr;
  }

  Count dependency_count = 0;
  if (!read_count(reader, dependency_count) ||
      dependency_count >
          (reader.get_size() - reader.get_location()) / Count(3)) {
    return nullptr;
  }

  Managed::Vector<Dependency> dependencies(arena);
  dependencies.reset(dependency_count);
  for (Count i = 0; i < dependency_count; i++) {
    View::Bytes dependency_name;
    Unsigned_64 dependency_major = 0;
    Unsigned_64 dependency_minor = 0;
    if (!read_bytes(arena, reader, dependency_name) ||
        !read_size(reader, dependency_major) ||
        !read_size(reader, dependency_minor) ||
        dependency_major > Unsigned_16(-1) ||
        dependency_minor > Unsigned_16(-1)) {
      return nullptr;
    }

    dependencies.insert(Dependency(
        dependency_name,
        Version(Unsigned_16(dependency_major), Unsigned_16(dependency_minor))));
  }

  Manifest& manifest = arena.construct<Manifest>(
      name, Version(Unsigned_16(major), Unsigned_16(minor)),
      dependencies.get_view());
  if (reader.get_location() != reader.get_size() || !manifest.is_valid(arena)) {
    return nullptr;
  }

  return &manifest;
}

static auto manifests_equal(const Manifest& left, const Manifest& right)
    -> Bool {
  if (left.get_name() != right.get_name() ||
      left.get_version() != right.get_version() ||
      left.get_dependencies().get_size() !=
          right.get_dependencies().get_size()) {
    return False;
  }

  for (Count i = 0; i < left.get_dependencies().get_size(); i++) {
    if (left.get_dependencies()[i] != right.get_dependencies()[i]) {
      return False;
    }
  }

  return True;
}

static auto get_child_count(const DecodedRecord& record) -> Count {
  if (record.code == Format::RecordCode::Namespace) {
    return record.exports.get_size();
  }
  if (record.reference == Format::ReferenceCode::Local) {
    return 1;
  }

  return 0;
}

static auto get_child(const DecodedRecord& record, Count index) -> Count {
  return record.code == Format::RecordCode::Namespace ? record.exports[index]
                                                      : record.target;
}

static auto visit_records(
    Allocator::Arena& arena,
    View::Vector<DecodedRecord> records) -> Bool {
  if (records.is_empty()) {
    return True;
  }

  Unsigned_8* states = arena.allocate(records.get_size());
  auto* stack =
      Data::cast<Count>(arena.allocate(sizeof(Count) * records.get_size()));
  auto* next_edges =
      Data::cast<Count>(arena.allocate(sizeof(Count) * records.get_size()));
  for (Count i = 0; i < records.get_size(); i++) {
    states[i] = 0;
  }

  for (Count root = 0; root < records.get_size(); root++) {
    if (states[root] == 2) {
      continue;
    }

    Count depth = 0;
    stack[0] = root;
    next_edges[0] = 0;
    while (true) {
      Count selected = stack[depth];
      if (states[selected] == 0) {
        states[selected] = 1;
      }

      const DecodedRecord& record = records[selected];
      if (next_edges[depth] == get_child_count(record)) {
        states[selected] = 2;
        if (depth == 0) {
          break;
        }

        depth--;
        continue;
      }

      Count child = get_child(record, next_edges[depth]++);
      if (child >= records.get_size() || states[child] == 1) {
        return False;
      }
      if (states[child] == 2) {
        continue;
      }

      depth++;
      stack[depth] = child;
      next_edges[depth] = 0;
    }
  }

  return True;
}

static auto validate_export_scope(
    Count scope,
    View::Vector<Count> exports,
    View::Vector<DecodedRecord> records,
    Managed::Map<ScopedName, Bool>& names) -> Bool {
  for (Count i = 0; i < exports.get_size(); i++) {
    if (exports[i] >= records.get_size()) {
      return False;
    }

    ScopedName identity(scope, records[exports[i]].name);
    if (names.find(identity) != nullptr) {
      return False;
    }

    names.insert(identity, True);
  }

  return True;
}

static auto find_restored(View::Vector<const Abstract*> restored, Count index)
    -> const Abstract& {
  if (index >= restored.get_size() || restored[index] == nullptr) {
    return Invalid::get_invalid();
  }

  return *restored[index];
}

auto Tetrodotoxin::Archiver::Reader::read_manifest(
    Allocator::Arena& arena) const -> const Manifest* {
  Static::Vector<View::Bytes, Format::section_count> sections;
  if (!read_sections(source, sections)) {
    return nullptr;
  }

  return read_manifest_section(
      arena, sections[Count(Format::Section::Manifest)]);
}

auto Tetrodotoxin::Archiver::Reader::read_package(
    Allocator::Arena& arena,
    const Manifest& manifest,
    View::Vector<Reference<Tetrodotoxin::Model::Package>> dependencies) const
    -> const Abstract& {
  Static::Vector<View::Bytes, Format::section_count> sections;
  if (!read_sections(source, sections)) {
    return Invalid::get_invalid();
  }

  Managed::Vector<View::Bytes> strings(arena);
  Bool read_string_table =
      read_strings(arena, sections[Count(Format::Section::Strings)], strings);
  if (!read_string_table) {
    return Invalid::get_invalid();
  }

  const Manifest* encoded_manifest =
      read_manifest_section(arena, sections[Count(Format::Section::Manifest)]);
  if (encoded_manifest == nullptr ||
      !manifests_equal(*encoded_manifest, manifest) ||
      dependencies.get_size() != manifest.get_dependencies().get_size()) {
    return Invalid::get_invalid();
  }

  BinaryReader terminal_reader(sections[Count(Format::Section::Terminals)]);
  Count terminal_count = 0;
  if (!read_count(terminal_reader, terminal_count) ||
      terminal_count >
          (terminal_reader.get_size() - terminal_reader.get_location()) /
              Count(2)) {
    return Invalid::get_invalid();
  }

  Managed::Vector<Tetrodotoxin::Model::Terminal> terminals(arena);
  terminals.reset(terminal_count);
  Managed::Map<View::Bytes, Bool> terminal_paths(arena);
  terminal_paths.ensure_capacity(terminal_count);
  for (Count i = 0; i < terminal_count; i++) {
    View::Bytes path;
    Unsigned_64 content_size = 0;
    if (!read_string(terminal_reader, strings.get_view(), path) ||
        !read_size(terminal_reader, content_size) ||
        content_size >
            terminal_reader.get_size() - terminal_reader.get_location()) {
      return Invalid::get_invalid();
    }

    View::Bytes content = terminal_reader.read_bytes(Count(content_size));
    if (terminal_reader.get_location() == Count(-1) ||
        !Tetrodotoxin::Model::Terminal::is_valid_path(path) ||
        terminal_paths.find(path) != nullptr) {
      return Invalid::get_invalid();
    }

    terminal_paths.insert(path, True);
    terminals.insert(Tetrodotoxin::Model::Terminal(arena, path, content));
  }
  if (terminal_reader.get_location() != terminal_reader.get_size()) {
    return Invalid::get_invalid();
  }

  BinaryReader graph_reader(sections[Count(Format::Section::Graph)]);
  View::Vector<View::Bytes> root_documentation;
  View::Vector<Count> root_exports;
  if (!read_documentation(
          arena, graph_reader, strings.get_view(), root_documentation) ||
      !read_indices(arena, graph_reader, root_exports)) {
    return Invalid::get_invalid();
  }

  Count record_count = 0;
  if (!read_count(graph_reader, record_count) ||
      record_count > graph_reader.get_size() - graph_reader.get_location()) {
    return Invalid::get_invalid();
  }

  Managed::Vector<DecodedRecord> records(arena);
  records.reset(record_count);
  for (Count i = 0; i < record_count; i++) {
    DecodedRecord record;
    Unsigned_8 code = graph_reader.read_unsigned_8();
    if (graph_reader.get_location() == Count(-1) ||
        (code != Unsigned_8(Format::RecordCode::Namespace) &&
         code != Unsigned_8(Format::RecordCode::Alias))) {
      return Invalid::get_invalid();
    }

    record.code = Format::RecordCode(code);
    if (!read_string(graph_reader, strings.get_view(), record.name) ||
        record.name.is_empty() ||
        !read_documentation(
            arena, graph_reader, strings.get_view(), record.documentation)) {
      return Invalid::get_invalid();
    }

    if (record.code == Format::RecordCode::Namespace) {
      if (!read_indices(arena, graph_reader, record.exports)) {
        return Invalid::get_invalid();
      }

      records.insert(record);
      continue;
    }

    Unsigned_8 reference = graph_reader.read_unsigned_8();
    if (graph_reader.get_location() == Count(-1) ||
        (reference != Unsigned_8(Format::ReferenceCode::Local) &&
         reference != Unsigned_8(Format::ReferenceCode::Dependency) &&
         reference !=
             Unsigned_8(Format::ReferenceCode::DependencyDefinition))) {
      return Invalid::get_invalid();
    }

    record.reference = Format::ReferenceCode(reference);
    if (record.reference == Format::ReferenceCode::Local) {
      if (!read_count(graph_reader, record.target)) {
        return Invalid::get_invalid();
      }
    } else {
      if (!read_count(graph_reader, record.dependency)) {
        return Invalid::get_invalid();
      }

      if (record.reference == Format::ReferenceCode::DependencyDefinition &&
          !read_count(graph_reader, record.target)) {
        return Invalid::get_invalid();
      }
    }

    records.insert(record);
  }

  if (graph_reader.get_location() != graph_reader.get_size()) {
    return Invalid::get_invalid();
  }

  Count export_count = root_exports.get_size();
  for (Count i = 0; i < record_count; i++) {
    if (records[i].code == Format::RecordCode::Namespace) {
      export_count += records[i].exports.get_size();
    }
  }
  Managed::Map<ScopedName, Bool> export_names(arena);
  export_names.ensure_capacity(export_count);
  if (!validate_export_scope(
          Count(-1), root_exports, records.get_view(), export_names)) {
    return Invalid::get_invalid();
  }

  for (Count i = 0; i < record_count; i++) {
    const DecodedRecord& record = records[i];
    if (record.code == Format::RecordCode::Namespace) {
      if (!validate_export_scope(
              i, record.exports, records.get_view(), export_names)) {
        return Invalid::get_invalid();
      }
      continue;
    }

    if (record.reference == Format::ReferenceCode::Local) {
      if (record.target >= record_count ||
          records[record.target].code != Format::RecordCode::Namespace) {
        return Invalid::get_invalid();
      }
    } else {
      if (record.dependency >= dependencies.get_size()) {
        return Invalid::get_invalid();
      }
      if (record.reference == Format::ReferenceCode::DependencyDefinition &&
          record.target >=
              dependencies[record.dependency].get().get_definition_count()) {
        return Invalid::get_invalid();
      }
    }
  }

  if (!visit_records(arena, records.get_view())) {
    return Invalid::get_invalid();
  }

  Managed::Vector<const Abstract*> restored(arena);
  restored.reset(record_count);
  for (Count i = 0; i < record_count; i++) {
    restored.insert(nullptr);
  }
  for (Count i = 0; i < record_count; i++) {
    if (records[i].code != Format::RecordCode::Namespace) {
      continue;
    }

    auto& documentation = arena.construct<Ttx::Model::Documentations::Block>(
        records[i].documentation);
    auto& namespace_object = arena.construct<Tetrodotoxin::Model::Namespace>(
        arena, records[i].name, documentation);
    restored[i] = &namespace_object;
  }

  for (Count i = 0; i < record_count; i++) {
    if (records[i].code != Format::RecordCode::Alias) {
      continue;
    }

    const Abstract* target = &Invalid::get_invalid();
    if (records[i].reference == Format::ReferenceCode::Local) {
      target = &find_restored(restored.get_view(), records[i].target);
    } else if (records[i].reference == Format::ReferenceCode::Dependency) {
      target = &dependencies[records[i].dependency].get();
    } else {
      target = &dependencies[records[i].dependency].get().get_definition(
          records[i].target);
    }
    if (target->is<Invalid>()) {
      return Invalid::get_invalid();
    }

    auto& documentation = arena.construct<Ttx::Model::Documentations::Block>(
        records[i].documentation);
    auto& alias = arena.construct<Ttx::Model::Alias>(
        records[i].name, *target, documentation);
    restored[i] = &alias;
  }

  for (Count i = 0; i < record_count; i++) {
    if (records[i].code != Format::RecordCode::Namespace) {
      continue;
    }

    const Abstract& abstract = find_restored(restored.get_view(), i);
    auto& namespace_object = const_cast<Tetrodotoxin::Model::Namespace&>(
        abstract.assume<Tetrodotoxin::Model::Namespace>());
    for (Count k = 0; k < records[i].exports.get_size(); k++) {
      const Abstract& child =
          find_restored(restored.get_view(), records[i].exports[k]);
      Bool published = namespace_object.add_export(child);
      if (!published) {
        return Invalid::get_invalid();
      }
    }
  }

  auto& root_documentation_object =
      arena.construct<Ttx::Model::Documentations::Block>(root_documentation);
  auto& root = arena.construct<Tetrodotoxin::Model::Namespace>(
      arena, View::Bytes(), root_documentation_object);
  for (Count i = 0; i < root_exports.get_size(); i++) {
    const Abstract& child = find_restored(restored.get_view(), root_exports[i]);
    Bool published = root.add_export(child);
    if (!published) {
      return Invalid::get_invalid();
    }
  }

  Managed::Vector<Reference<Abstract>> definitions(arena);
  definitions.reset(record_count);
  for (Count i = 0; i < record_count; i++) {
    definitions.insert(
        Reference<Abstract>(find_restored(restored.get_view(), i)));
  }

  return arena.construct<Tetrodotoxin::Model::Packages::Precompiled>(
      arena, root, dependencies, definitions.get_view(), terminals.get_view());
}
