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

using PackageWriter = Stream::Binary<Data::ByteOrder::Little, Managed::Bytes>;
using PatchWriter = Perimortem::Core::Writer::Binary<Data::ByteOrder::Little>;

class Context {
 public:
  Context(Allocator::Arena& arena, Managed::Bytes& output)
      : output(output),
        writer(output),
        local_ids(arena),
        external_refs(arena),
        local_types(arena) {}

  auto write(const Package& package, View::Vector<Reference> references)
      -> Bool;

 private:
  auto is_builtin_type(const Ttx::Type* type) const -> Bool;
  auto collect_references(View::Vector<Reference> references) -> Bool;
  auto collect_external_refs(Reference reference, Count reference_id) -> Bool;
  auto is_external_type(const Ttx::Type* type) const -> Bool;
  auto collect_local_type(const Ttx::Type* type) -> void;
  auto write_ref(const Ttx::Type* type) -> Bool;
  auto write_documentation(Ttx::Documentation documentation) -> void;
  auto write_attributes(View::Vector<Ttx::Attribute> attributes) -> void;
  auto write_members(View::Vector<Ttx::Member> members) -> Bool;
  auto write_type(const Ttx::Type& type) -> Bool;
  auto write_imports(View::Vector<Dependency> imports) -> Bool;
  auto write_references(View::Vector<Reference> references) -> Bool;
  auto write_terminals(View::Vector<Terminal> terminals) -> void;
  auto write_linkages(View::Vector<Tetrodotoxin::Compiler::Linkage> linkages)
      -> Bool;

  Managed::Bytes& output;
  PackageWriter writer;
  Managed::Map<const Ttx::Type*, Count> local_ids;
  Managed::Map<const Ttx::Type*, Type::Reference> external_refs;
  Managed::Vector<const Ttx::Type*> local_types;
};

static auto write_header(PackageWriter& writer) -> void {
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

static auto write_size(PackageWriter& writer, Count value) -> void {
  do {
    Bits_8 byte = Bits_8(value & 0x7f);
    value >>= 7;
    if (value != 0) {
      byte |= 0x80;
    }

    writer << byte;
  } while (value != 0);
}

static auto write_bytes(PackageWriter& writer, View::Bytes value) -> void {
  write_size(writer, value.get_size());
  writer << value;
}

static auto write_version(PackageWriter& writer, Version value) -> void {
  writer << value.get_value()[0] << value.get_value()[1];
}

auto Context::is_builtin_type(const Ttx::Type* type) const -> Bool {
  View::Vector<const Ttx::Type*> types =
      Tetrodotoxin::Standard::Types::get_types();
  for (Count i = 0; i < types.get_size(); i++) {
    if (type == types[i]) {
      return True;
    }
  }

  return False;
}

auto Context::collect_references(View::Vector<Reference> references) -> Bool {
  external_refs.clear();
  for (Count i = 0; i < references.get_size(); i++) {
    if (!collect_external_refs(references[i], i)) {
      return False;
    }
  }

  return True;
}

auto Context::collect_external_refs(Reference reference, Count reference_id)
    -> Bool {
  if (!reference.is_valid()) {
    return False;
  }

  View::Vector<const Ttx::Type*> types = reference.get_types();
  if (types.is_empty()) {
    return False;
  }

  for (Count i = 0; i < types.get_size(); i++) {
    external_refs.insert(types[i], Type::Reference(reference_id, i));
  }

  return True;
}

auto Context::is_external_type(const Ttx::Type* type) const -> Bool {
  return type != nullptr && external_refs.contains(type);
}

auto Context::collect_local_type(const Ttx::Type* type) -> void {
  if (type == nullptr || is_builtin_type(type) || is_external_type(type) ||
      local_ids.contains(type)) {
    return;
  }

  local_ids.insert(type, local_types.get_size());
  local_types.insert(type);

  collect_local_type(type->get_alias_parent());

  View::Vector<Ttx::Member> members = type->get_members();
  for (Count i = 0; i < members.get_size(); i++) {
    collect_local_type(&members[i].get_type());
  }

  View::Vector<const Ttx::Type*> nested = type->get_types();
  for (Count i = 0; i < nested.get_size(); i++) {
    collect_local_type(nested[i]);
  }

  View::Vector<Ttx::Function> functions = type->get_functions();
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

auto Context::write_ref(const Ttx::Type* type) -> Bool {
  if (type == nullptr) {
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

auto Context::write_documentation(Ttx::Documentation documentation) -> void {
  View::Vector<View::Bytes> lines = documentation.get_lines();
  write_size(writer, lines.get_size());
  for (Count i = 0; i < lines.get_size(); i++) {
    write_bytes(writer, lines[i]);
  }
}

auto Context::write_attributes(View::Vector<Ttx::Attribute> attributes)
    -> void {
  write_size(writer, attributes.get_size());
  for (Count i = 0; i < attributes.get_size(); i++) {
    write_bytes(writer, attributes[i].get_key());
    write_bytes(writer, attributes[i].get_value());
  }
}

auto Context::write_members(View::Vector<Ttx::Member> members) -> Bool {
  write_size(writer, members.get_size());
  for (Count i = 0; i < members.get_size(); i++) {
    write_bytes(writer, members[i].get_name());
    if (!write_ref(&members[i].get_type())) {
      return False;
    }

    writer << (members[i].is_defaulted() ? Bits_8(1) : Bits_8(0));
    write_documentation(members[i].get_documentation());
    write_attributes(members[i].get_attributes());
  }

  return True;
}

auto Context::write_type(const Ttx::Type& type) -> Bool {
  write_bytes(writer, type.get_name());
  write_documentation(type.get_documentation());

  write_attributes(type.get_attributes());
  if (!write_ref(type.get_alias_parent())) {
    return False;
  }

  if (!write_members(type.get_members())) {
    return False;
  }

  View::Vector<const Ttx::Type*> nested = type.get_types();
  write_size(writer, nested.get_size());
  for (Count i = 0; i < nested.get_size(); i++) {
    auto* id = local_ids.find(nested[i]);
    if (id == nullptr) {
      return False;
    }

    write_size(writer, id->value);
  }

  View::Vector<Ttx::Function> functions = type.get_functions();
  write_size(writer, functions.get_size());
  for (Count i = 0; i < functions.get_size(); i++) {
    write_bytes(writer, functions[i].get_name());
    write_documentation(functions[i].get_documentation());
    if (!write_members(functions[i].get_parameters().get_members())) {
      return False;
    }

    if (!write_members(functions[i].get_result().get_members())) {
      return False;
    }
  }

  return True;
}

auto Context::write_imports(View::Vector<Dependency> imports) -> Bool {
  write_size(writer, imports.get_size());
  for (Count i = 0; i < imports.get_size(); i++) {
    if (!imports[i].is_valid()) {
      return False;
    }

    write_bytes(writer, imports[i].get_local_name());
    write_bytes(writer, imports[i].get_source_name());
    write_version(writer, imports[i].get_version());
  }

  return True;
}

auto Context::write_references(View::Vector<Reference> references) -> Bool {
  write_size(writer, references.get_size());
  for (Count i = 0; i < references.get_size(); i++) {
    if (!references[i].is_valid()) {
      return False;
    }

    write_bytes(writer, references[i].get_source_name());
    write_version(writer, references[i].get_version());
  }

  return True;
}

auto Context::write_terminals(View::Vector<Terminal> terminals) -> void {
  write_size(writer, terminals.get_size());
  for (Count i = 0; i < terminals.get_size(); i++) {
    write_bytes(writer, terminals[i].get_group());
    write_bytes(writer, terminals[i].get_path());
    write_bytes(writer, terminals[i].get_content());
  }
}

auto Context::write_linkages(
    View::Vector<Tetrodotoxin::Compiler::Linkage> linkages) -> Bool {
  write_size(writer, linkages.get_size());
  for (Count i = 0; i < linkages.get_size(); i++) {
    if (!linkages[i].is_valid() || !write_ref(&linkages[i].get_owner())) {
      return False;
    }

    View::Vector<Ttx::Function> functions =
        linkages[i].get_owner().get_functions();
    Count function = Count(-1);
    for (Count k = 0; k < functions.get_size(); k++) {
      if (&functions[k] == &linkages[i].get_function()) {
        function = k;
        break;
      }
    }

    if (function == Count(-1)) {
      return False;
    }

    write_size(writer, function);
    write_bytes(writer, linkages[i].get_symbol());
  }

  return True;
}

auto Context::write(const Package& package, View::Vector<Reference> references)
    -> Bool {
  if (!collect_references(references)) {
    return False;
  }

  const Ttx::Type& root = package.get_type();
  collect_local_type(&root);
  View::Vector<const Ttx::Type*> local_roots = package.get_types();
  for (Count i = 0; i < local_roots.get_size(); i++) {
    collect_local_type(local_roots[i]);
  }

  View::Vector<Tetrodotoxin::Compiler::Linkage> linkages =
      package.get_linkages();
  for (Count i = 0; i < linkages.get_size(); i++) {
    collect_local_type(&linkages[i].get_owner());
  }

  auto* root_id = local_ids.find(&root);
  if (root_id == nullptr) {
    return False;
  }

  write_header(writer);
  Count manifest_offset = output.get_size();
  write_bytes(writer, package.get_manifest().get_name());
  write_version(writer, Tetrodotoxin::Standard::Types::get_version());
  if (!write_imports(package.get_manifest().get_imports())) {
    return False;
  }

  Count references_offset = output.get_size();
  if (!write_references(references)) {
    return False;
  }

  Count package_offset = output.get_size();
  write_size(writer, root_id->value);
  write_size(writer, local_types.get_size());
  for (Count i = 0; i < local_types.get_size(); i++) {
    if (!write_type(*local_types[i])) {
      return False;
    }
  }

  write_terminals(package.get_terminals());

  Count linkages_offset = output.get_size();
  if (!write_linkages(linkages)) {
    return False;
  }

  PatchWriter patch(output.get_access());
  write_table_offset(patch, Format::Table::Manifest, manifest_offset);
  write_table_offset(patch, Format::Table::References, references_offset);
  write_table_offset(patch, Format::Table::Package, package_offset);
  write_table_offset(patch, Format::Table::Linkages, linkages_offset);

  patch.set_pointer(Format::version_offset);
  patch << Hash(output.get_view().slice(manifest_offset)).get_value();
  patch << Hash(output.get_view().slice(package_offset)).get_value();
  return patch.is_valid();
}

auto Tetrodotoxin::Archiver::Writer::write(
    Allocator::Arena& arena,
    const Package& package,
    View::Vector<Reference> references) -> View::Bytes {
  Managed::Bytes output(arena);
  Context context(arena, output);
  if (!context.write(package, references)) {
    return View::Bytes();
  }

  return output.get_view();
}
