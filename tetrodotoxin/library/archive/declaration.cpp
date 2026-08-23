// Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/library/archive/declaration.hpp"

#include "perimortem/memory/managed/bytes.hpp"
#include "perimortem/memory/managed/vector.hpp"

#include "perimortem/serialization/stream/textual.hpp"

#include "tetrodotoxin/library/archive/reference.hpp"
#include "tetrodotoxin/library/archive/value.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Ttx::Concept;
using namespace Tetrodotoxin;

enum class AttributeValue : U8 {
  Empty,
  Bytes,
  Unsigned,
  Signed,
  Real,
  Flag,
};

static auto write_attribute(
    Library::Archive::Writer& writer,
    const Tetrodotoxin::Language::Attribute& attribute) -> Bool {
  BAIL_IF(!writer.write(attribute.get_key()));

  const auto& value = attribute.get_value();
  return value.visit(
      [&]() -> Bool {
        writer.write(U8(AttributeValue::Empty));
        return True;
      },
      [&](View::Bytes selected) -> Bool {
        writer.write(U8(AttributeValue::Bytes));
        return writer.write(selected);
      },
      [&](U64 selected) -> Bool {
        writer.write(U8(AttributeValue::Unsigned));
        writer.write(selected);
        return True;
      },
      [&](S64 selected) -> Bool {
        writer.write(U8(AttributeValue::Signed));
        writer.write(selected);
        return True;
      },
      [&](R64 selected) -> Bool {
        writer.write(U8(AttributeValue::Real));
        writer.write(selected);
        return True;
      },
      [&](Bool selected) -> Bool {
        writer.write(U8(AttributeValue::Flag));
        writer.write(U8(selected ? 1 : 0));
        return True;
      });
}

static auto read_attribute(
    Library::Archive::Reader& reader,
    Allocator::Arena& arena) -> Option<Tetrodotoxin::Language::Attribute> {
  auto key = reader.read_bytes();
  auto kind = reader.read_u8();
  BAIL_IF(!key || key->is_empty() || !kind);

  Tetrodotoxin::Language::Attribute::Value value;
  switch (AttributeValue(*kind)) {
  case AttributeValue::Empty:
    break;
  case AttributeValue::Bytes: {
    auto selected = reader.read_bytes();
    BAIL_IF(!selected);
    value = Tetrodotoxin::Language::Attribute::Value(arena.proxy(*selected));
    break;
  }
  case AttributeValue::Unsigned: {
    auto selected = reader.read_u64();
    BAIL_IF(!selected);
    value = Tetrodotoxin::Language::Attribute::Value(*selected);
    break;
  }
  case AttributeValue::Signed: {
    auto selected = reader.read_s64();
    BAIL_IF(!selected);
    value = Tetrodotoxin::Language::Attribute::Value(*selected);
    break;
  }
  case AttributeValue::Real: {
    auto selected = reader.read_r64();
    BAIL_IF(!selected);
    value = Tetrodotoxin::Language::Attribute::Value(*selected);
    break;
  }
  case AttributeValue::Flag: {
    auto selected = reader.read_u8();
    BAIL_IF(!selected || *selected > 1);
    value =
        Tetrodotoxin::Language::Attribute::Value(*selected == 1 ? True : False);
    break;
  }
  default:
    return {};
  }

  return Tetrodotoxin::Language::Attribute::create_synthetic(
      arena.proxy(*key), value);
}

auto Library::Archive::Declaration::read(
    Reader& reader,
    Allocator::Arena& arena) -> Option<Declaration> {
  auto documentation = reader.read_documentation(arena);
  auto encoded_visibility = reader.read_u8();
  auto name = reader.read_bytes();
  auto attribute_count = reader.read_u32();
  BAIL_IF(
      !documentation || !encoded_visibility || !name || name->is_empty() ||
      !attribute_count ||
      *encoded_visibility > U8(Tetrodotoxin::Language::Visibility::Exposed));

  Managed::Vector<Tetrodotoxin::Language::Attribute> attributes(arena);
  for (Count index = 0; index < *attribute_count; index++) {
    auto attribute = read_attribute(reader, arena);
    BAIL_IF(!attribute);
    attributes.insert(*attribute);
  }

  return Declaration(
      *documentation, attributes.get_view(), arena.proxy(*name),
      Tetrodotoxin::Language::Visibility(*encoded_visibility));
}

auto Library::Archive::Declaration::write(Writer& writer) const -> Bool {
  BAIL_IF(!writer.write(documentation) || attributes.get_size() > U32(-1));

  writer.write(U8(visibility));
  BAIL_IF(!writer.write(name));
  writer.write(U32(attributes.get_size()));
  for (const Tetrodotoxin::Language::Attribute& attribute : attributes) {
    BAIL_IF(!write_attribute(writer, attribute));
  }
  return True;
}

auto Library::Archive::Declaration::create_definition(
    Allocator::Arena& arena,
    Abstract& host) const -> Tetrodotoxin::Language::Definition& {
  return Tetrodotoxin::Language::Definition::create_restored(
      arena, documentation, host, attributes, name, visibility);
}

auto Library::Archive::write(
    Writer& writer,
    const Library::Language::Alias& alias) -> Bool {
  auto record = writer.begin(Tag::Alias);
  Declaration declaration(alias.get_definition());
  return declaration.write(writer) &&
         Archive::write(writer, alias.get_target_reference()) &&
         writer.finish(record);
}

auto Library::Archive::read_alias(
    Reader& reader,
    Allocator::Arena& arena,
    Abstract& host) -> Option<Library::Language::Alias&> {
  auto record = reader.read_record();
  BAIL_IF(
      !record || record->get_tag() != U16(Tag::Alias) || record->is_optional());

  Reader contents(record->get_payload());
  auto declaration = Declaration::read(contents, arena);
  auto target = read_type_reference(contents, arena, host);
  BAIL_IF(!declaration || !target || !contents.is_complete());

  auto& definition = declaration->create_definition(arena, host);
  return Library::Language::Alias::create(arena, definition, *target);
}

auto Library::Archive::write(
    Writer& writer,
    const Library::Language::Field& field) -> Bool {
  auto record = writer.begin(Tag::Field);
  Declaration declaration(field.get_definition());
  BAIL_IF(!declaration.write(writer));

  writer.write(U8(field.get_writability()));
  auto reference = field.get_type_reference();
  writer.write(U8(reference ? 1 : 0));
  BAIL_IF(reference && !Archive::write(writer, *reference));

  Bool include_constant =
      field.get_writability() == Library::Language::Writability::Constant;
  auto folded = include_constant ? field.get_constant()
                                 : Option<Library::Language::Model::Pack&>();
  writer.write(U8(include_constant ? 1 : 0));
  BAIL_IF(include_constant && (!folded || !write_folded(writer, *folded)));
  return writer.finish(record);
}

auto Library::Archive::write_field_slot(
    Writer& writer,
    const Library::Language::Field& field,
    Count ordinal) -> Bool {
  auto record = writer.begin(Tag::FieldSlot);
  writer.write(U64(ordinal));
  auto reference = field.get_type_reference();
  writer.write(U8(reference ? 1 : 0));
  BAIL_IF(reference && !Archive::write(writer, *reference));
  return writer.finish(record);
}

auto Library::Archive::read_field(
    Reader& reader,
    Allocator::Arena& arena,
    Abstract& host) -> Option<Library::Language::Field&> {
  auto record = reader.read_record();
  BAIL_IF(
      !record || record->get_tag() != U16(Tag::Field) || record->is_optional());

  Reader contents(record->get_payload());
  auto declaration = Declaration::read(contents, arena);
  auto encoded_writability = contents.read_u8();
  auto has_type = contents.read_u8();
  BAIL_IF(
      !declaration || !encoded_writability ||
      *encoded_writability > U8(Library::Language::Writability::Constant) ||
      !has_type || *has_type > 1);

  Option<Library::Language::TypeReference> type_reference;
  if (*has_type == 1) {
    auto restored = read_type_reference(contents, arena, host);
    BAIL_IF(!restored);
    type_reference = *restored;
  }

  auto has_initializer = contents.read_u8();
  BAIL_IF(!has_initializer || *has_initializer > 1);
  Option<Library::Language::Model::Pack&> initializer;
  if (*has_initializer == 1) {
    initializer = read_folded(contents, arena, host);
    BAIL_IF(!initializer);
  }
  BAIL_IF(!contents.is_complete());

  auto& definition = declaration->create_definition(arena, host);
  return Library::Language::Field::create(
      arena, definition, Library::Language::Writability(*encoded_writability),
      type_reference, initializer);
}

auto Library::Archive::read_field_slot(
    Reader& reader,
    Allocator::Arena& arena,
    Abstract& host,
    Count ordinal) -> Option<Library::Language::Field&> {
  auto record = reader.read_record();
  BAIL_IF(
      !record || record->get_tag() != U16(Tag::FieldSlot) ||
      record->is_optional());

  Reader contents(record->get_payload());
  auto encoded_ordinal = contents.read_u64();
  auto has_type = contents.read_u8();
  BAIL_IF(
      !encoded_ordinal || *encoded_ordinal != ordinal || !has_type ||
      *has_type > 1);

  Option<Library::Language::TypeReference> type_reference;
  if (*has_type == 1) {
    auto restored = read_type_reference(contents, arena, host);
    BAIL_IF(!restored);
    type_reference = *restored;
  }
  BAIL_IF(!contents.is_complete());

  Managed::Bytes name(arena, "$slot"_view);
  Perimortem::Serialization::Stream::Textual<Managed::Bytes> stream(name);
  stream << ordinal;
  auto& definition = Tetrodotoxin::Language::Definition::create_synthetic(
      arena, Documentation::get_empty(), host, name.get_view(),
      Tetrodotoxin::Language::Visibility::Private,
      Ttx::Lexical::Anchor::create(Ttx::Lexical::Span()));
  return Library::Language::Field::create(
      arena, definition, Library::Language::Writability::Internal,
      type_reference, Option<Library::Language::Model::Pack&>());
}

auto Library::Archive::write(
    Writer& writer,
    const Library::Language::Function& function) -> Bool {
  auto record = writer.begin(Tag::Function);
  Declaration declaration(function.get_definition());
  return declaration.write(writer) &&
         Archive::write(writer, function.get_signature()) &&
         writer.finish(record);
}

auto Library::Archive::read_function(
    Reader& reader,
    Allocator::Arena& arena,
    Abstract& host) -> Option<Library::Language::Function&> {
  auto record = reader.read_record();
  BAIL_IF(
      !record || record->get_tag() != U16(Tag::Function) ||
      record->is_optional());

  Reader contents(record->get_payload());
  auto declaration = Declaration::read(contents, arena);
  auto signature = read_signature(contents, arena, host);
  BAIL_IF(!declaration || !signature || !contents.is_complete());

  auto& definition = declaration->create_definition(arena, host);
  return Library::Language::Function::create(arena, definition, *signature);
}
