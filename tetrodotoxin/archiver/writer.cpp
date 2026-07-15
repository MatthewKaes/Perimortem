// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/archiver/writer.hpp"

#include "perimortem/core/hash.hpp"
#include "perimortem/core/writer/binary.hpp"

#include "perimortem/memory/allocator/arena.hpp"
#include "perimortem/memory/managed/bytes.hpp"
#include "perimortem/memory/managed/map.hpp"
#include "perimortem/memory/managed/vector.hpp"

#include "perimortem/serialization/stream/binary.hpp"

#include "tetrodotoxin/archiver/format.hpp"
#include "tetrodotoxin/archiver/type/reference.hpp"
#include "tetrodotoxin/standard/types.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Perimortem::Serialization;
using namespace Tetrodotoxin::Archiver;

using BinaryStream = Stream::Binary<Data::ByteOrder::Little, Managed::Bytes>;
using PatchWriter = Perimortem::Core::Writer::Binary<Data::ByteOrder::Little>;

class PackageWriter {
 public:
  PackageWriter(Allocator::Arena& arena, Managed::Bytes& output)
      : output(output),
        writer(output),
        local_ids(arena),
        external_refs(arena),
        local_types(arena),
        linkage_by_function(arena),
        linkage_owners(arena) {}

  auto write(
      View::Bytes package_name,
      View::Vector<Dependency> imports,
      const Ttx::Type& root_type,
      View::Vector<const Ttx::Type*> types,
      View::Vector<Terminal> terminals,
      View::Vector<Tetrodotoxin::Abi::Linkage> linkages,
      View::Vector<const Package*> references) -> Bool;

 private:
  auto is_builtin_type(const Ttx::Type* type) const -> Bool;
  auto collect_references(View::Vector<const Package*> references) -> Bool;
  auto collect_external_refs(const Package& package, Count reference_id)
      -> Bool;
  auto is_external_type(const Ttx::Type* type) const -> Bool;
  auto collect_functions(View::Vector<Ttx::Function> functions) -> void;
  auto collect_local_type(const Ttx::Type* type) -> void;
  auto write_ref(const Ttx::Type* type) -> Bool;
  auto write_documentation(Ttx::Documentation documentation) -> void;
  auto write_attributes(View::Vector<Ttx::Attribute> attributes) -> void;
  auto write_members(View::Vector<Ttx::Member> members) -> Bool;
  auto write_functions(View::Vector<Ttx::Function> functions) -> Bool;
  auto write_type(const Ttx::Type& type) -> Bool;
  auto write_imports(View::Vector<Dependency> imports) -> Bool;
  auto write_references(View::Vector<const Package*> references) -> Bool;
  auto write_terminals(View::Vector<Terminal> terminals) -> void;
  auto write_function_linkages(
      const Ttx::Type& owner,
      View::Vector<Ttx::Function> functions,
      Count& written) -> Bool;
  auto write_linkages(View::Vector<Tetrodotoxin::Abi::Linkage> linkages)
      -> Bool;

  Managed::Bytes& output;
  BinaryStream writer;
  Managed::Map<const Ttx::Type*, Count> local_ids;
  Managed::Map<const Ttx::Type*, Type::Reference> external_refs;
  Managed::Vector<const Ttx::Type*> local_types;
  Managed::Map<const Ttx::Function*, const Tetrodotoxin::Abi::Linkage*>
      linkage_by_function;
  Managed::Vector<const Ttx::Type*> linkage_owners;
};

static auto write_header(BinaryStream& writer) -> void {
  writer << Format::magic << Format::format_version;
  writer << Bits_64(0) << Bits_64(0);
  for (Count i = 0; i < Format::table_count; i++) {
    writer << Bits_64(0);
  }
}

static auto write_table_offset(
    PatchWriter& writer,
    Format::Table table,
    Count offset) -> void {
  writer.set_pointer(Format::slot(table));
  writer << Bits_64(offset);
}

static auto write_size(BinaryStream& writer, Count value) -> void {
  do {
    Bits_8 byte = Bits_8(value & 0x7f);
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

static auto write_uuid(BinaryStream& writer, Perimortem::System::Uuid value)
    -> void {
  writer << value.get_value()[0] << value.get_value()[1];
}

auto PackageWriter::is_builtin_type(const Ttx::Type* type) const -> Bool {
  View::Vector<const Ttx::Type*> types =
      Tetrodotoxin::Standard::Types::get_types();
  for (Count i = 0; i < types.get_size(); i++) {
    if (type == types[i]) {
      return True;
    }
  }

  return False;
}

auto PackageWriter::collect_references(View::Vector<const Package*> references)
    -> Bool {
  external_refs.clear();
  for (Count i = 0; i < references.get_size(); i++) {
    if (references[i] == nullptr) {
      return False;
    }

    Bool collected = collect_external_refs(*references[i], i);
    if (!collected) {
      return False;
    }
  }

  return True;
}

auto PackageWriter::collect_external_refs(
    const Package& package,
    Count reference_id) -> Bool {
  View::Vector<const Ttx::Type*> types = package.get_types();
  const Manifest& manifest = package.get_manifest();
  if (manifest.get_name().is_empty() || !manifest.get_version().is_set() ||
      types.is_empty()) {
    return False;
  }

  for (Count i = 0; i < types.get_size(); i++) {
    external_refs.insert(types[i], Type::Reference(reference_id, i));
  }

  return True;
}

auto PackageWriter::is_external_type(const Ttx::Type* type) const -> Bool {
  return type != nullptr && external_refs.contains(type);
}

auto PackageWriter::collect_functions(View::Vector<Ttx::Function> functions)
    -> void {
  for (Count i = 0; i < functions.get_size(); i++) {
    View::Vector<Ttx::Member> parameters =
        functions[i].get_parameters().get_members();
    for (Count k = 0; k < parameters.get_size(); k++) {
      collect_local_type(&parameters[k].get_type());
    }

    View::Vector<Ttx::Member> results = functions[i].get_result().get_members();
    for (Count k = 0; k < results.get_size(); k++) {
      collect_local_type(&results[k].get_type());
    }
  }
}

auto PackageWriter::collect_local_type(const Ttx::Type* type) -> void {
  if (type == nullptr || type->is_invalid() || is_builtin_type(type) ||
      is_external_type(type) || local_ids.contains(type)) {
    return;
  }

  local_ids.insert(type, local_types.get_size());
  local_types.insert(type);

  collect_local_type(&type->get_alias_parent());

  View::Vector<Ttx::Member> members = type->get_members();
  for (Count i = 0; i < members.get_size(); i++) {
    collect_local_type(&members[i].get_type());
  }

  View::Vector<Ttx::Type::Reference> nested = type->get_types();
  for (Count i = 0; i < nested.get_size(); i++) {
    collect_local_type(&nested[i].get_type());
  }

  collect_functions(type->get_type_functions());
  collect_functions(type->get_addressable_functions());
}

auto PackageWriter::write_ref(const Ttx::Type* type) -> Bool {
  if (type == nullptr || type->is_invalid()) {
    writer << static_cast<Bits_8>(Type::Reference::Kind::None);
    return True;
  }

  if (is_builtin_type(type)) {
    writer << static_cast<Bits_8>(Type::Reference::Kind::Builtin);
    write_bytes(writer, type->get_name());
    return True;
  }

  if (auto* local = local_ids.find(type)) {
    writer << static_cast<Bits_8>(Type::Reference::Kind::Local);
    write_size(writer, local->value);
    return True;
  }

  if (auto* external = external_refs.find(type)) {
    writer << static_cast<Bits_8>(Type::Reference::Kind::Package);
    write_size(writer, external->value.get_reference_id());
    write_size(writer, external->value.get_type_id());
    return True;
  }

  return False;
}

auto PackageWriter::write_documentation(Ttx::Documentation documentation)
    -> void {
  View::Vector<View::Bytes> lines = documentation.get_lines();
  write_size(writer, lines.get_size());
  for (Count i = 0; i < lines.get_size(); i++) {
    write_bytes(writer, lines[i]);
  }
}

auto PackageWriter::write_attributes(View::Vector<Ttx::Attribute> attributes)
    -> void {
  write_size(writer, attributes.get_size());
  for (Count i = 0; i < attributes.get_size(); i++) {
    write_bytes(writer, attributes[i].get_key());
    writer << Bits_8(attributes[i].get_kind());
    switch (attributes[i].get_kind()) {
    case Ttx::Attribute::Kind::Empty:
      break;
    case Ttx::Attribute::Kind::Bytes:
      write_bytes(writer, attributes[i].get_bytes());
      break;
    case Ttx::Attribute::Kind::Unsigned:
      writer << attributes[i].get_unsigned();
      break;
    case Ttx::Attribute::Kind::Signed:
      writer << attributes[i].get_signed();
      break;
    case Ttx::Attribute::Kind::Real:
      writer << attributes[i].get_real();
      break;
    case Ttx::Attribute::Kind::Boolean:
      writer << attributes[i].get_boolean().value;
      break;
    }
  }
}

auto PackageWriter::write_members(View::Vector<Ttx::Member> members) -> Bool {
  write_size(writer, members.get_size());
  for (Count i = 0; i < members.get_size(); i++) {
    write_bytes(writer, members[i].get_name());
    Bool wrote_type = write_ref(&members[i].get_type());
    if (!wrote_type) {
      return False;
    }

    writer << (members[i].is_defaulted() ? Bits_8(1) : Bits_8(0));
    write_documentation(members[i].get_documentation());
    write_attributes(members[i].get_attributes());
  }

  return True;
}

auto PackageWriter::write_functions(View::Vector<Ttx::Function> functions)
    -> Bool {
  write_size(writer, functions.get_size());
  for (Count i = 0; i < functions.get_size(); i++) {
    write_bytes(writer, functions[i].get_name());
    write_documentation(functions[i].get_documentation());
    Bool wrote_parameters =
        write_members(functions[i].get_parameters().get_members());
    if (!wrote_parameters) {
      return False;
    }

    Bool wrote_results = write_members(functions[i].get_result().get_members());
    if (!wrote_results) {
      return False;
    }
  }

  return True;
}

auto PackageWriter::write_type(const Ttx::Type& type) -> Bool {
  write_bytes(writer, type.get_name());
  write_documentation(type.get_documentation());
  write_attributes(type.get_attributes());
  Bool wrote_alias = write_ref(&type.get_alias_parent());
  if (!wrote_alias) {
    return False;
  }

  Bool wrote_members = write_members(type.get_members());
  if (!wrote_members) {
    return False;
  }

  View::Vector<Ttx::Type::Reference> nested = type.get_types();
  write_size(writer, nested.get_size());
  for (Count i = 0; i < nested.get_size(); i++) {
    auto* id = local_ids.find(&nested[i].get_type());
    if (id == nullptr) {
      return False;
    }

    write_size(writer, id->value);
  }

  Bool wrote_type_functions = write_functions(type.get_type_functions());
  if (!wrote_type_functions) {
    return False;
  }

  Bool wrote_addressable_functions =
      write_functions(type.get_addressable_functions());
  return wrote_addressable_functions;
}

auto PackageWriter::write_imports(View::Vector<Dependency> imports) -> Bool {
  write_size(writer, imports.get_size());
  for (Count i = 0; i < imports.get_size(); i++) {
    if (imports[i].get_local_name().is_empty() ||
        imports[i].get_source_name().is_empty() ||
        !imports[i].get_version().is_set()) {
      return False;
    }

    write_bytes(writer, imports[i].get_local_name());
    write_bytes(writer, imports[i].get_source_name());
    write_uuid(writer, imports[i].get_version());
  }

  return True;
}

auto PackageWriter::write_references(View::Vector<const Package*> references)
    -> Bool {
  write_size(writer, references.get_size());
  for (Count i = 0; i < references.get_size(); i++) {
    if (references[i] == nullptr) {
      return False;
    }

    const Package& package = *references[i];
    const Manifest& manifest = package.get_manifest();
    if (manifest.get_name().is_empty() || !manifest.get_version().is_set() ||
        package.get_types().is_empty()) {
      return False;
    }

    write_bytes(writer, manifest.get_name());
    write_uuid(writer, manifest.get_version());
  }

  return True;
}

auto PackageWriter::write_terminals(View::Vector<Terminal> terminals) -> void {
  write_size(writer, terminals.get_size());
  for (Count i = 0; i < terminals.get_size(); i++) {
    write_bytes(writer, terminals[i].get_group());
    write_bytes(writer, terminals[i].get_path());
    write_bytes(writer, terminals[i].get_content());
  }
}

auto PackageWriter::write_function_linkages(
    const Ttx::Type& owner,
    View::Vector<Ttx::Function> functions,
    Count& written) -> Bool {
  Count count = 0;
  for (Count i = 0; i < functions.get_size(); i++) {
    const auto* entry = linkage_by_function.find(&functions[i]);
    if (entry == nullptr) {
      continue;
    }

    if (&entry->value->get_owner() != &owner) {
      return False;
    }

    count++;
  }

  write_size(writer, count);
  for (Count i = 0; i < functions.get_size(); i++) {
    const auto* entry = linkage_by_function.find(&functions[i]);
    if (entry == nullptr) {
      continue;
    }

    write_size(writer, i);
    write_bytes(writer, entry->value->get_symbol());
    written++;
  }

  return True;
}

auto PackageWriter::write_linkages(
    View::Vector<Tetrodotoxin::Abi::Linkage> linkages) -> Bool {
  linkage_by_function.clear();
  linkage_owners.clear();
  for (Count i = 0; i < linkages.get_size(); i++) {
    const Ttx::Function* function = &linkages[i].get_function();
    if (linkages[i].get_symbol().is_empty() ||
        linkage_by_function.find(function) != nullptr) {
      return False;
    }

    linkage_by_function.insert(function, &linkages[i]);
    const Ttx::Type* owner = &linkages[i].get_owner();
    if (!linkage_owners.contains(owner)) {
      linkage_owners.insert(owner);
    }
  }

  Count written = 0;
  write_size(writer, linkage_owners.get_size());
  for (Count i = 0; i < linkage_owners.get_size(); i++) {
    const Ttx::Type& owner = *linkage_owners[i];
    Bool wrote_owner = write_ref(&owner);
    if (!wrote_owner) {
      return False;
    }

    Bool wrote_type =
        write_function_linkages(owner, owner.get_type_functions(), written);
    if (!wrote_type) {
      return False;
    }

    Bool wrote_addressable = write_function_linkages(
        owner, owner.get_addressable_functions(), written);
    if (!wrote_addressable) {
      return False;
    }
  }

  return written == linkages.get_size();
}

auto PackageWriter::write(
    View::Bytes package_name,
    View::Vector<Dependency> imports,
    const Ttx::Type& root,
    View::Vector<const Ttx::Type*> types,
    View::Vector<Terminal> terminals,
    View::Vector<Tetrodotoxin::Abi::Linkage> linkages,
    View::Vector<const Package*> references) -> Bool {
  Bool collected_references = collect_references(references);
  if (!collected_references) {
    return False;
  }

  collect_local_type(&root);

  for (Count i = 0; i < types.get_size(); i++) {
    collect_local_type(types[i]);
  }

  for (Count i = 0; i < linkages.get_size(); i++) {
    collect_local_type(&linkages[i].get_owner());
  }

  auto* root_id = local_ids.find(&root);
  if (root_id == nullptr) {
    return False;
  }

  write_header(writer);

  Count manifest_offset = output.get_size();
  write_bytes(writer, package_name);
  write_uuid(writer, Tetrodotoxin::Standard::Types::get_version());
  Bool wrote_imports = write_imports(imports);
  if (!wrote_imports) {
    return False;
  }

  Count references_offset = output.get_size();
  Bool wrote_references = write_references(references);
  if (!wrote_references) {
    return False;
  }

  Count package_offset = output.get_size();
  write_size(writer, root_id->value);
  write_size(writer, local_types.get_size());
  for (Count i = 0; i < local_types.get_size(); i++) {
    Bool wrote_type = write_type(*local_types[i]);
    if (!wrote_type) {
      return False;
    }
  }

  write_terminals(terminals);

  Count linkages_offset = output.get_size();
  Bool wrote_linkages = write_linkages(linkages);
  if (!wrote_linkages) {
    return False;
  }

  PatchWriter patch(output.get_access());
  write_table_offset(patch, Format::Table::Manifest, manifest_offset);
  write_table_offset(patch, Format::Table::References, references_offset);
  write_table_offset(patch, Format::Table::Package, package_offset);
  write_table_offset(patch, Format::Table::Linkages, linkages_offset);

  patch.set_pointer(Format::content_id_offset);
  patch << Hash(output.get_view().slice(manifest_offset)).get_value();
  patch << Hash(output.get_view().slice(package_offset)).get_value();
  return patch.is_valid();
}

auto Tetrodotoxin::Archiver::Writer::write(
    Allocator::Arena& arena,
    View::Bytes package_name,
    View::Vector<Dependency> imports,
    const Ttx::Type& root_type,
    View::Vector<const Ttx::Type*> types,
    View::Vector<Terminal> terminals,
    View::Vector<Tetrodotoxin::Abi::Linkage> linkages,
    View::Vector<const Package*> references) -> View::Bytes {
  Managed::Bytes output(arena);
  PackageWriter writer(arena, output);
  Bool wrote = writer.write(
      package_name, imports, root_type, types, terminals, linkages, references);
  if (!wrote) {
    return View::Bytes();
  }

  return output.get_view();
}
