// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/archive/declaration.hpp"

#include "perimortem/memory/managed/vector.hpp"

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
    const Language::Attribute& attribute) -> Bool {
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
    Allocator::Arena& arena) -> Option<Language::Attribute> {
  auto key = reader.read_bytes();
  auto kind = reader.read_u8();
  BAIL_IF(!key || key->is_empty() || !kind);

  Language::Attribute::Value value;
  switch (AttributeValue(*kind)) {
  case AttributeValue::Empty:
    break;
  case AttributeValue::Bytes: {
    auto selected = reader.read_bytes();
    BAIL_IF(!selected);
    value = Language::Attribute::Value(arena.proxy(*selected));
    break;
  }
  case AttributeValue::Unsigned: {
    auto selected = reader.read_u64();
    BAIL_IF(!selected);
    value = Language::Attribute::Value(*selected);
    break;
  }
  case AttributeValue::Signed: {
    auto selected = reader.read_s64();
    BAIL_IF(!selected);
    value = Language::Attribute::Value(*selected);
    break;
  }
  case AttributeValue::Real: {
    auto selected = reader.read_r64();
    BAIL_IF(!selected);
    value = Language::Attribute::Value(*selected);
    break;
  }
  case AttributeValue::Flag: {
    auto selected = reader.read_u8();
    BAIL_IF(!selected || *selected > 1);
    value = Language::Attribute::Value(*selected == 1 ? True : False);
    break;
  }
  default:
    return {};
  }

  return Language::Attribute::create_synthetic(arena.proxy(*key), value);
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
      *encoded_visibility > U8(Language::Visibility::Exposed));

  Managed::Vector<Language::Attribute> attributes(arena);
  for (Count index = 0; index < *attribute_count; index++) {
    auto attribute = read_attribute(reader, arena);
    BAIL_IF(!attribute);
    attributes.insert(*attribute);
  }

  return Declaration(
      *documentation, attributes.get_view(), arena.proxy(*name),
      Language::Visibility(*encoded_visibility));
}

auto Library::Archive::Declaration::write(Writer& writer) const -> Bool {
  BAIL_IF(!writer.write(documentation) || attributes.get_size() > U32(-1));

  writer.write(U8(visibility));
  BAIL_IF(!writer.write(name));
  writer.write(U32(attributes.get_size()));
  for (const Language::Attribute& attribute : attributes) {
    BAIL_IF(!write_attribute(writer, attribute));
  }
  return True;
}

auto Library::Archive::Declaration::create_definition(
    Allocator::Arena& arena,
    Abstract& host) const -> Language::Definition& {
  return Language::Definition::create_restored(
      arena, documentation, host, attributes, name, visibility);
}
