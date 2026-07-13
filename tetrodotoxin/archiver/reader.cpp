// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/archiver/reader.hpp"

#include "perimortem/core/static/vector.hpp"
#include "perimortem/core/reader/binary.hpp"

#include "perimortem/memory/managed/map.hpp"
#include "perimortem/memory/managed/vector.hpp"

#include "tetrodotoxin/archiver/format.hpp"
#include "tetrodotoxin/archiver/package.hpp"
#include "tetrodotoxin/archiver/type/reference.hpp"
#include "tetrodotoxin/standard/types.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Tetrodotoxin::Archiver;

using PackageReader = Perimortem::Core::Reader::Binary<Data::ByteOrder::Little>;

class Restore {
 public:
  Restore(
      Allocator::Arena& arena,
      View::Bytes source,
      View::Bytes linkages,
      Manifest manifest,
      View::Vector<Reference> references)
      : arena(arena),
        manifest(manifest),
        references(references),
        reader(source),
        linkage_reader(linkages),
        local_types(arena),
        terminals(arena),
        linkages(arena) {}

  static auto read_table(
      View::Bytes source,
      Format::Table table,
      Version& archive_version) -> View::Bytes {
    PackageReader reader(source);
    if (source.get_size() < Format::data_offset ||
        reader.read_bytes(Format::magic.get_size()) != Format::magic ||
        reader.read_bits_32() != Format::format_version) {
      return View::Bytes();
    }

    archive_version = Version(reader.read_bits_64(), reader.read_bits_64());

    Static::Vector<Bits_64, Format::table_count> offsets;
    for (Count i = 0; i < Format::table_count; i++) {
      offsets[i] = reader.read_bits_64();
      Count offset = Count(offsets[i]);
      if (reader.get_location() == Count(-1) || offset < Format::data_offset ||
          offset >= source.get_size() ||
          (i != 0 && offset <= Count(offsets[i - 1]))) {
        return View::Bytes();
      }
    }

    Count table_id = Count(table);
    Count start = Count(offsets[table_id]);
    Count end = table_id + 1 < Format::table_count
                    ? Count(offsets[table_id + 1])
                    : source.get_size();
    if (start >= end || end > source.get_size()) {
      return View::Bytes();
    }

    return source.slice(start, end - start);
  }

  static auto read_manifest(
      Allocator::Arena& arena,
      View::Bytes source,
      Version version) -> Manifest {
    PackageReader reader(source);
    View::Bytes name = read_bytes(reader);
    Version standard_version = read_version(reader);

    Managed::Vector<Dependency> imports(arena);
    Count import_count = read_size(reader);
    imports.reset(import_count);
    for (Count i = 0; i < import_count; i++) {
      View::Bytes local_name = read_bytes(reader);
      View::Bytes source_name = read_bytes(reader);
      imports.insert(Dependency(local_name, source_name, read_version(reader)));
    }

    if (standard_version != Tetrodotoxin::Standard::Types::get_version() ||
        !version.is_set() || reader.get_location() != reader.get_size()) {
      return Manifest();
    }

    return Manifest(name, version, imports);
  }

  static auto resolve_references(
      Allocator::Arena& arena,
      View::Bytes source,
      View::Vector<Reference> available,
      Managed::Vector<Reference>& resolved) -> Bool {
    Managed::Map<View::Bytes, Reference> by_name(arena);
    for (Count i = 0; i < available.get_size(); i++) {
      if (available[i].is_valid()) {
        by_name.insert(available[i].get_source_name(), available[i]);
      }
    }

    PackageReader reader(source);
    Count reference_count = read_size(reader);
    resolved.reset(reference_count);
    for (Count i = 0; i < reference_count; i++) {
      View::Bytes name = read_bytes(reader);
      Version version = read_version(reader);
      const auto* match = by_name.find(name);
      if (match == nullptr || match->value.get_version() != version) {
        return False;
      }

      resolved.insert(match->value);
    }

    return reader.get_location() == reader.get_size();
  }

  auto read() -> Package* {
    if (!manifest.is_valid()) {
      return nullptr;
    }

    Count root_id = read_size(reader);
    Count type_count = read_size(reader);
    local_types.reset(type_count);
    // Local references are ids into this table. Allocate every slot before
    // reading the type bodies so aliases and members can point forward without
    // losing TTX address identity.
    for (Count i = 0; i < type_count; i++) {
      local_types.insert(&arena.allocate<Ttx::Type>());
    }

    for (Count i = 0; i < local_types.get_size(); i++) {
      if (!restore_type(*local_types[i], local_types)) {
        return nullptr;
      }
    }

    if (reader.get_location() == Count(-1) || root_id >= type_count ||
        !read_terminals() || !read_linkages()) {
      return nullptr;
    }

    Managed::Vector<const Ttx::Type*> package_types(arena);
    package_types.reset(local_types.get_size());
    for (Count i = 0; i < local_types.get_size(); i++) {
      package_types.insert(local_types[i]);
    }

    View::Vector<Dependency> source_imports = manifest.get_imports();
    Managed::Vector<Dependency> imports(arena);
    imports.reset(source_imports.get_size());
    for (Count i = 0; i < source_imports.get_size(); i++) {
      imports.insert(source_imports[i]);
    }

    Manifest restored_manifest(
        manifest.get_name(), manifest.get_version(), imports);
    return &arena.construct<Package>(
        restored_manifest, *local_types[root_id], package_types.get_view(),
        terminals.get_view(), linkages.get_view());
  }

 private:
  static auto read_size(PackageReader& reader) -> Count {
    Count value = 0;
    Count shift = 0;
    while (shift < sizeof(Count) * 8) {
      Bits_8 byte = reader.read_bits_8();
      if (reader.get_location() == Count(-1)) {
        return Count();
      }

      value |= Count(byte & 0x7f) << shift;
      if ((byte & 0x80) == 0) {
        return value;
      }

      shift += 7;
    }

    reader.set_location(Count(-1));
    return Count();
  }

  static auto read_bytes(PackageReader& reader) -> View::Bytes {
    return reader.read_bytes(read_size(reader));
  }

  static auto read_version(PackageReader& reader) -> Version {
    Bits_64 high = reader.read_bits_64();
    Bits_64 low = reader.read_bits_64();
    return Version(high, low);
  }

  auto read_ref(PackageReader& source, View::Vector<Ttx::Type*> local_types)
      -> const Ttx::Type* {
    auto kind = Type::Reference::Kind(source.read_bits_8());
    switch (kind) {
    case Type::Reference::Kind::None:
      return nullptr;

    case Type::Reference::Kind::Builtin:
      return Tetrodotoxin::Standard::Types::find_type(read_bytes(source));

    case Type::Reference::Kind::Local: {
      Count local_id = read_size(source);
      return local_id < local_types.get_size() ? local_types[local_id]
                                               : nullptr;
    }

    case Type::Reference::Kind::Package: {
      Count reference_id = read_size(source);
      Count type_id = read_size(source);
      if (reference_id >= references.get_size()) {
        return nullptr;
      }

      const Reference& reference = references[reference_id];
      if (!reference.is_valid()) {
        return nullptr;
      }

      View::Vector<const Ttx::Type*> types = reference.get_types();
      return type_id < types.get_size() ? types[type_id] : nullptr;
    }
    }

    return nullptr;
  }

  auto read_documentation() -> Ttx::Documentation {
    Managed::Vector<View::Bytes> lines(arena);
    Count line_count = read_size(reader);
    lines.reset(line_count);
    for (Count i = 0; i < line_count; i++) {
      lines.insert(read_bytes(reader));
    }

    return Ttx::Documentation(lines);
  }

  auto read_attributes(Managed::Vector<Ttx::Attribute>& attributes) -> void {
    Count attribute_count = read_size(reader);
    attributes.reset(attribute_count);
    for (Count i = 0; i < attribute_count; i++) {
      attributes.insert(Ttx::Attribute(read_bytes(reader), read_bytes(reader)));
    }
  }

  auto read_members(
      Managed::Vector<Ttx::Member>& members,
      View::Vector<Ttx::Type*> local_types) -> Bool {
    Count member_count = read_size(reader);
    members.reset(member_count);
    for (Count i = 0; i < member_count; i++) {
      View::Bytes name = read_bytes(reader);
      const Ttx::Type* type = read_ref(reader, local_types);
      Bool defaulted = reader.read_bits_8() != 0;
      Ttx::Documentation documentation = read_documentation();
      Managed::Vector<Ttx::Attribute> attributes(arena);
      read_attributes(attributes);
      if (type == nullptr) {
        return False;
      }

      members.insert(
          Ttx::Member(
              name, *type, defaulted, documentation, attributes.get_view()));
    }

    return True;
  }

  auto read_nested(
      Managed::Vector<const Ttx::Type*>& nested,
      View::Vector<Ttx::Type*> local_types) -> Bool {
    Count nested_count = read_size(reader);
    nested.reset(nested_count);
    for (Count i = 0; i < nested_count; i++) {
      Count id = read_size(reader);
      if (id >= local_types.get_size()) {
        return False;
      }

      nested.insert(local_types[id]);
    }

    return True;
  }

  auto read_functions(
      Managed::Vector<Ttx::Function>& functions,
      View::Vector<Ttx::Type*> local_types) -> Bool {
    Count function_count = read_size(reader);
    functions.reset(function_count);
    for (Count i = 0; i < function_count; i++) {
      View::Bytes name = read_bytes(reader);
      Ttx::Documentation documentation = read_documentation();
      Managed::Vector<Ttx::Member> parameters(arena);
      Managed::Vector<Ttx::Member> results(arena);
      if (!read_members(parameters, local_types) ||
          !read_members(results, local_types)) {
        return False;
      }

      functions.insert(
          Ttx::Function(
              name, Ttx::Layout(parameters.get_view()),
              Ttx::Layout(results.get_view()), documentation));
    }

    return True;
  }

  auto read_terminals() -> Bool {
    Count terminal_count = read_size(reader);
    terminals.reset(terminal_count);
    for (Count i = 0; i < terminal_count; i++) {
      terminals.insert(
          Terminal(read_bytes(reader), read_bytes(reader), read_bytes(reader)));
    }

    return reader.get_location() == reader.get_size();
  }

  auto read_linkages() -> Bool {
    Count linkage_count = read_size(linkage_reader);
    linkages.reset(linkage_count);
    for (Count i = 0; i < linkage_count; i++) {
      const Ttx::Type* owner = read_ref(linkage_reader, local_types.get_view());
      Count function = read_size(linkage_reader);
      View::Bytes symbol = read_bytes(linkage_reader);
      if (owner == nullptr || function >= owner->get_functions().get_size() ||
          symbol.is_empty()) {
        return False;
      }

      linkages.insert(
          Tetrodotoxin::Compiler::Linkage(
              *owner, owner->get_functions()[function], symbol));
    }

    return linkage_reader.get_location() == linkage_reader.get_size();
  }

  auto restore_type(Ttx::Type& type, View::Vector<Ttx::Type*> local_types)
      -> Bool {
    View::Bytes name = read_bytes(reader);
    Ttx::Documentation documentation = read_documentation();

    Managed::Vector<Ttx::Attribute> attributes(arena);
    read_attributes(attributes);

    const Ttx::Type* alias_parent = read_ref(reader, local_types);

    Managed::Vector<Ttx::Member> members(arena);
    Managed::Vector<const Ttx::Type*> nested(arena);
    Managed::Vector<Ttx::Function> functions(arena);
    if (!read_members(members, local_types) ||
        !read_nested(nested, local_types) ||
        !read_functions(functions, local_types)) {
      return False;
    }

    if (alias_parent != nullptr) {
      new (&type) Ttx::Type(
          Ttx::Type::alias(name, *alias_parent, documentation, attributes));
      return True;
    }

    new (&type)
        Ttx::Type(name, members, nested, functions, documentation, attributes);
    return True;
  }

  Allocator::Arena& arena;
  Manifest manifest;
  View::Vector<Reference> references;
  PackageReader reader;
  PackageReader linkage_reader;
  Managed::Vector<Ttx::Type*> local_types;
  Managed::Vector<Terminal> terminals;
  Managed::Vector<Tetrodotoxin::Compiler::Linkage> linkages;
};

auto Tetrodotoxin::Archiver::Reader::read_manifest(
    Allocator::Arena& arena) const -> Manifest {
  Version version;
  View::Bytes manifest_table =
      Restore::read_table(source, Format::Table::Manifest, version);
  return Restore::read_manifest(arena, manifest_table, version);
}

auto Tetrodotoxin::Archiver::Reader::read_package(
    Allocator::Arena& arena,
    Manifest manifest,
    View::Vector<Reference> references) const -> Package* {
  Version reference_version;
  View::Bytes reference_table =
      Restore::read_table(source, Format::Table::References, reference_version);
  Managed::Vector<Reference> resolved_references(arena);
  if (reference_version != manifest.get_version() ||
      !Restore::resolve_references(
          arena, reference_table, references, resolved_references)) {
    return nullptr;
  }

  Version package_version;
  View::Bytes package_table =
      Restore::read_table(source, Format::Table::Package, package_version);
  if (package_version != manifest.get_version()) {
    return nullptr;
  }

  Version linkage_version;
  View::Bytes linkage_table =
      Restore::read_table(source, Format::Table::Linkages, linkage_version);
  if (linkage_version != manifest.get_version()) {
    return nullptr;
  }

  return Restore(
             arena, package_table, linkage_table, manifest,
             resolved_references.get_view())
      .read();
}
