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
using namespace Perimortem::System;
using namespace Tetrodotoxin::Archiver;

using BinaryReader = Perimortem::Core::Reader::Binary<Data::ByteOrder::Little>;

class PackageReader {
 public:
  PackageReader(
      Allocator::Arena& arena,
      View::Bytes source,
      View::Bytes linkages,
      const Manifest& manifest,
      View::Vector<const Package*> references)
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
      Uuid& content_id) -> View::Bytes {
    BinaryReader reader(source);
    if (source.get_size() < Format::data_offset) {
      return View::Bytes();
    }

    View::Bytes magic = reader.read_bytes(Format::magic.get_size());
    if (magic != Format::magic) {
      return View::Bytes();
    }

    Unsigned_32 format_version = reader.read_unsigned_32();
    if (format_version != Format::format_version) {
      return View::Bytes();
    }

    content_id = Uuid(reader.read_unsigned_64(), reader.read_unsigned_64());

    Static::Vector<Unsigned_64, Format::table_count> offsets;
    for (Count i = 0; i < Format::table_count; i++) {
      offsets[i] = reader.read_unsigned_64();
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
      Uuid version) -> const Manifest* {
    BinaryReader reader(source);
    View::Bytes name = read_bytes(reader);
    Uuid standard_version = read_uuid(reader);

    Managed::Vector<Dependency> imports(arena);
    Count import_count = read_size(reader);
    imports.reset(import_count);
    for (Count i = 0; i < import_count; i++) {
      View::Bytes local_name = read_bytes(reader);
      View::Bytes source_name = read_bytes(reader);
      Uuid import_version = read_uuid(reader);
      if (local_name.is_empty() || source_name.is_empty() ||
          !import_version.is_set()) {
        return nullptr;
      }

      imports.insert(Dependency(local_name, source_name, import_version));
    }

    Bool valid =
        !name.is_empty() && version.is_set() &&
        standard_version == Tetrodotoxin::Standard::Types::get_version() &&
        reader.get_location() == reader.get_size();
    if (!valid) {
      return nullptr;
    }

    return &arena.construct<Manifest>(name, version, imports.get_view());
  }

  static auto resolve_references(
      Allocator::Arena& arena,
      View::Bytes source,
      View::Vector<const Package*> available,
      Managed::Vector<const Package*>& resolved) -> Bool {
    Managed::Map<View::Bytes, const Package*> by_name(arena);
    for (Count i = 0; i < available.get_size(); i++) {
      if (available[i] == nullptr) {
        return False;
      }

      by_name.insert(available[i]->get_manifest().get_name(), available[i]);
    }

    BinaryReader reader(source);
    Count reference_count = read_size(reader);
    resolved.reset(reference_count);
    for (Count i = 0; i < reference_count; i++) {
      View::Bytes name = read_bytes(reader);
      Uuid version = read_uuid(reader);
      const auto* match = by_name.find(name);
      if (match == nullptr ||
          match->value->get_manifest().get_version() != version) {
        return False;
      }

      resolved.insert(match->value);
    }

    return reader.get_location() == reader.get_size();
  }

  auto read() -> Package* {
    Count root_id = read_size(reader);
    Count type_count = read_size(reader);
    local_types.reset(type_count);
    // Local references are ids into this table. Allocate every slot before
    // reading the type bodies so aliases and members can point forward without
    // losing TTX address identity.
    for (Count i = 0; i < type_count; i++) {
      local_types.insert(arena.reserve<Ttx::Type>());
    }

    for (Count i = 0; i < local_types.get_size(); i++) {
      Bool restored = restore_type(local_types[i], local_types);
      if (!restored) {
        return nullptr;
      }
    }

    if (reader.get_location() == Count(-1) || root_id >= type_count) {
      return nullptr;
    }

    Bool read_terminal_table = read_terminals();
    if (!read_terminal_table) {
      return nullptr;
    }

    Bool read_linkage_table = read_linkages();
    if (!read_linkage_table) {
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
  static auto read_size(BinaryReader& reader) -> Count {
    Count value = 0;
    Count shift = 0;
    while (shift < sizeof(Count) * 8) {
      Unsigned_8 byte = reader.read_unsigned_8();
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

  static auto read_bytes(BinaryReader& reader) -> View::Bytes {
    return reader.read_bytes(read_size(reader));
  }

  static auto read_uuid(BinaryReader& reader) -> Uuid {
    Unsigned_64 high = reader.read_unsigned_64();
    Unsigned_64 low = reader.read_unsigned_64();
    return Uuid(high, low);
  }

  auto read_ref(BinaryReader& source, View::Vector<Ttx::Type*> local_types)
      -> const Ttx::Type& {
    auto kind = Type::Reference::Kind(source.read_unsigned_8());
    if (source.get_location() == Count(-1)) {
      return Ttx::Type::invalid();
    }

    switch (kind) {
    case Type::Reference::Kind::None:
      return TypeReference::none();

    case Type::Reference::Kind::Builtin: {
      View::Bytes name = read_bytes(source);
      return source.get_location() == Count(-1)
                 ? TypeReference::invalid()
                 : TypeReference::resolved(
                       Tetrodotoxin::Standard::Types::find_type(name));
    }

    case Type::Reference::Kind::Local: {
      Count local_id = read_size(source);
      return source.get_location() != Count(-1) &&
                     local_id < local_types.get_size()
                 ? TypeReference::resolved(local_types[local_id])
                 : TypeReference::invalid();
    }

    case Type::Reference::Kind::Package: {
      Count reference_id = read_size(source);
      Count type_id = read_size(source);
      if (source.get_location() == Count(-1) ||
          reference_id >= references.get_size()) {
        return TypeReference::invalid();
      }

      const Package& package = *references[reference_id];
      View::Vector<const Ttx::Type*> types = package.get_types();
      return type_id < types.get_size()
                 ? TypeReference::resolved(types[type_id])
                 : TypeReference::invalid();
    }
    }

    return TypeReference::invalid();
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

  auto read_attribute(Managed::Vector<Ttx::Attribute>& attributes) -> Bool {
    View::Bytes key = read_bytes(reader);
    auto kind = Ttx::Attribute::Kind(reader.read_unsigned_8());
    if (reader.get_location() == Count(-1)) {
      return False;
    }

    switch (kind) {
    case Ttx::Attribute::Kind::Empty:
      attributes.insert(Ttx::Attribute(key));
      return True;
    case Ttx::Attribute::Kind::Bytes: {
      View::Bytes value = read_bytes(reader);
      if (reader.get_location() == Count(-1)) {
        return False;
      }

      attributes.insert(Ttx::Attribute(key, value));
      return True;
    }
    case Ttx::Attribute::Kind::Unsigned: {
      Unsigned_64 value = reader.read_unsigned_64();
      if (reader.get_location() == Count(-1)) {
        return False;
      }

      attributes.insert(Ttx::Attribute(key, value));
      return True;
    }
    case Ttx::Attribute::Kind::Signed: {
      Signed_64 value = reader.read_signed_64();
      if (reader.get_location() == Count(-1)) {
        return False;
      }

      attributes.insert(Ttx::Attribute(key, value));
      return True;
    }
    case Ttx::Attribute::Kind::Real: {
      Real_64 value = reader.read_real_64();
      if (reader.get_location() == Count(-1)) {
        return False;
      }

      attributes.insert(Ttx::Attribute(key, value));
      return True;
    }
    case Ttx::Attribute::Kind::Boolean: {
      Unsigned_8 value = reader.read_unsigned_8();
      if (reader.get_location() == Count(-1) || value > 1) {
        return False;
      }

      attributes.insert(Ttx::Attribute(key, Bool(value != 0)));
      return True;
    }
    }

    return False;
  }

  auto read_attributes(Managed::Vector<Ttx::Attribute>& attributes) -> Bool {
    Count attribute_count = read_size(reader);
    attributes.reset(attribute_count);
    for (Count i = 0; i < attribute_count; i++) {
      Bool read = read_attribute(attributes);
      if (!read) {
        return False;
      }
    }

    return True;
  }

  auto read_members(
      Managed::Vector<Ttx::Member>& members,
      View::Vector<Ttx::Type*> local_types) -> Bool {
    Count member_count = read_size(reader);
    members.reset(member_count);
    for (Count i = 0; i < member_count; i++) {
      View::Bytes name = read_bytes(reader);
      TypeReference type = read_ref(reader, local_types);
      Bool defaulted = reader.read_unsigned_8() != 0;
      Ttx::Documentation documentation = read_documentation();
      Managed::Vector<Ttx::Attribute> attributes(arena);
      Bool read_member_attributes = read_attributes(attributes);
      if (!read_member_attributes || type.is_none() || type.is_invalid()) {
        return False;
      }

      members.insert(
          Ttx::Member::reserved_type(
              name, type.get_storage(), defaulted, documentation,
              attributes.get_view()));
    }

    return True;
  }

  auto read_nested(
      Managed::Vector<Ttx::Type::Reference>& nested,
      View::Vector<Ttx::Type*> local_types) -> Bool {
    Count nested_count = read_size(reader);
    nested.reset(nested_count);
    for (Count i = 0; i < nested_count; i++) {
      Count id = read_size(reader);
      if (id >= local_types.get_size()) {
        return False;
      }

      nested.insert(Ttx::Type::Reference::reserved(local_types[id]));
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
      Bool read_parameters = read_members(parameters, local_types);
      if (!read_parameters) {
        return False;
      }

      Bool read_results = read_members(results, local_types);
      if (!read_results) {
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

  auto read_function_linkages(
      const Ttx::Type& owner,
      View::Vector<Ttx::Function> functions) -> Bool {
    Count linkage_count = read_size(linkage_reader);
    for (Count i = 0; i < linkage_count; i++) {
      Count function = read_size(linkage_reader);
      View::Bytes symbol = read_bytes(linkage_reader);
      if (function >= functions.get_size() || symbol.is_empty()) {
        return False;
      }

      linkages.insert(
          Tetrodotoxin::Abi::Linkage(owner, functions[function], symbol));
    }

    return True;
  }

  auto read_linkages() -> Bool {
    Count owner_count = read_size(linkage_reader);
    for (Count i = 0; i < owner_count; i++) {
      TypeReference owner = read_ref(linkage_reader, local_types.get_view());
      if (owner.is_none() || owner.is_invalid()) {
        return False;
      }

      Bool read_type = read_function_linkages(
          owner.get_type(), owner.get_type().get_type_functions());
      if (!read_type) {
        return False;
      }

      Bool read_addressable = read_function_linkages(
          owner.get_type(), owner.get_type().get_addressable_functions());
      if (!read_addressable) {
        return False;
      }
    }

    return linkage_reader.get_location() == linkage_reader.get_size();
  }

  auto restore_type(Ttx::Type* type, View::Vector<Ttx::Type*> local_types)
      -> Bool {
    View::Bytes name = read_bytes(reader);
    Ttx::Documentation documentation = read_documentation();

    Managed::Vector<Ttx::Attribute> attributes(arena);
    Bool read_type_attributes = read_attributes(attributes);
    if (!read_type_attributes) {
      return False;
    }

    TypeReference alias_parent = read_ref(reader, local_types);
    if (alias_parent.is_invalid()) {
      return False;
    }

    Managed::Vector<Ttx::Member> members(arena);
    Managed::Vector<Ttx::Type::Reference> nested(arena);
    Managed::Vector<Ttx::Function> type_functions(arena);
    Managed::Vector<Ttx::Function> addressable_functions(arena);
    Bool read_type_members = read_members(members, local_types);
    if (!read_type_members) {
      return False;
    }

    Bool read_nested_types = read_nested(nested, local_types);
    if (!read_nested_types) {
      return False;
    }

    Bool read_type_functions = read_functions(type_functions, local_types);
    if (!read_type_functions) {
      return False;
    }

    Bool read_addressable_functions =
        read_functions(addressable_functions, local_types);
    if (!read_addressable_functions) {
      return False;
    }

    for (Count i = 0; i < addressable_functions.get_size(); i++) {
      Ttx::Layout parameters = addressable_functions[i].get_parameters();
      if (parameters.is_empty() || !parameters.member_at(0).references(type)) {
        return False;
      }
    }

    if (!alias_parent.is_none()) {
      new (type) Ttx::Type(
          Ttx::Type::alias_reserved(
              name, alias_parent.get_storage(), documentation, attributes));
      return True;
    }

    new (type) Ttx::Type(
        name, members, nested, type_functions, addressable_functions,
        documentation, attributes);
    return True;
  }

  Allocator::Arena& arena;
  Manifest manifest;
  View::Vector<const Package*> references;
  BinaryReader reader;
  BinaryReader linkage_reader;
  Managed::Vector<Ttx::Type*> local_types;
  Managed::Vector<Terminal> terminals;
  Managed::Vector<Tetrodotoxin::Abi::Linkage> linkages;
};

auto Tetrodotoxin::Archiver::Reader::read_manifest(
    Allocator::Arena& arena) const -> const Manifest* {
  Uuid version;
  View::Bytes manifest_table =
      PackageReader::read_table(source, Format::Table::Manifest, version);
  return PackageReader::read_manifest(arena, manifest_table, version);
}

auto Tetrodotoxin::Archiver::Reader::read_package(
    Allocator::Arena& arena,
    const Manifest& manifest,
    View::Vector<const Package*> references) const -> Package* {
  Uuid reference_version;
  View::Bytes reference_table = PackageReader::read_table(
      source, Format::Table::References, reference_version);
  Managed::Vector<const Package*> resolved_references(arena);
  if (reference_version != manifest.get_version()) {
    return nullptr;
  }

  Bool resolved = PackageReader::resolve_references(
      arena, reference_table, references, resolved_references);
  if (!resolved) {
    return nullptr;
  }

  Uuid package_version;
  View::Bytes package_table = PackageReader::read_table(
      source, Format::Table::Package, package_version);
  if (package_version != manifest.get_version()) {
    return nullptr;
  }

  Uuid linkage_version;
  View::Bytes linkage_table = PackageReader::read_table(
      source, Format::Table::Linkages, linkage_version);
  if (linkage_version != manifest.get_version()) {
    return nullptr;
  }

  PackageReader reader(
      arena, package_table, linkage_table, manifest,
      resolved_references.get_view());
  return reader.read();
}
