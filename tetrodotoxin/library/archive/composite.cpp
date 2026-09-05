// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/library/archive/composite.hpp"

#include "tetrodotoxin/library/archive/declaration.hpp"
#include "tetrodotoxin/library/archive/reference.hpp"
#include "tetrodotoxin/library/language/alias.hpp"
#include "tetrodotoxin/library/language/field.hpp"
#include "tetrodotoxin/library/language/function.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Ttx::Concept;
using namespace Tetrodotoxin::Library;

static auto is_published(const Abstract& declaration) -> Bool {
  auto alias = declaration.select<Language::Alias>();
  if (alias) {
    return alias->get_definition().is_published();
  }
  auto field = declaration.select<Language::Field>();
  if (field) {
    return field->get_definition().is_published();
  }
  auto function = declaration.select<Language::Function>();
  if (function) {
    return function->get_definition().is_published();
  }
  auto composite = declaration.select<Language::Types::Composite>();
  if (composite) {
    return composite->get_definition().is_published();
  }
  auto enumeration = declaration.select<Language::Types::Enumeration>();
  return enumeration && enumeration->get_definition().is_published();
}

auto Archive::write_declarations(
    Writer& writer,
    const Language::Types::Composite& composite) -> Bool {
  for (const Abstract* retained : composite.get_declarations()) {
    const Abstract& declaration = *retained;
    auto alias = declaration.select<Language::Alias>();
    if (alias) {
      BAIL_IF(!Archive::write(writer, *alias));
      continue;
    }
    auto field = declaration.select<Language::Field>();
    if (field) {
      BAIL_IF(!Archive::write(writer, *field));
      continue;
    }
    auto function = declaration.select<Language::Function>();
    if (function) {
      BAIL_IF(!Archive::write(writer, *function));
      continue;
    }
    auto interface = declaration.select<Language::Types::Interface>();
    auto implemented = declaration.select<Language::Types::Implemented>();
    auto structure = declaration.select<Language::Types::Structure>();
    auto name_space = declaration.select<Language::Types::Namespace>();
    auto object = declaration.select<Language::Types::Object>();
    if (implemented) {
      BAIL_IF(!Archive::write(writer, *implemented));
      continue;
    }
    if (interface) {
      BAIL_IF(!Archive::write(writer, *interface));
      continue;
    }
    if (object) {
      BAIL_IF(!Archive::write(writer, *object));
      continue;
    }
    if (structure) {
      BAIL_IF(!Archive::write(writer, *structure));
      continue;
    }
    if (name_space) {
      BAIL_IF(!Archive::write(writer, *name_space));
      continue;
    }
    auto enumeration = declaration.select<Language::Types::Enumeration>();
    BAIL_IF(!enumeration || !Archive::write(writer, *enumeration));
  }
  return True;
}

auto Archive::read_declarations(
    Reader& reader,
    Allocator::Arena& arena,
    Language::Types::Composite& composite) -> Bool {
  while (!reader.is_complete()) {
    Reader probe = reader;
    auto record = probe.read_record();
    BAIL_IF(!record);

    if (record->is_optional()) {
      BAIL_IF(!reader.read_record());
      continue;
    }

    Option<Abstract&> restored;
    Language::Model::Completion* completion = nullptr;
    Language::Types::Composite::Category category =
        Language::Types::Composite::Category::Addressable;
    switch (Tag(record->get_tag())) {
    case Tag::Alias: {
      auto selected = read_alias(reader, arena, composite);
      BAIL_IF(!selected);
      restored = *selected;
      category = Language::Types::Composite::Category::Type;
      break;
    }
    case Tag::Field: {
      auto selected = read_field(reader, arena, composite);
      BAIL_IF(!selected);
      restored = *selected;
      break;
    }
    case Tag::Function: {
      auto selected = read_function(reader, arena, composite);
      BAIL_IF(!selected);
      restored = *selected;
      category = Language::Types::Composite::Category::Callable;
      break;
    }
    case Tag::Structure: {
      auto selected = read_structure(reader, arena, composite);
      BAIL_IF(!selected);
      restored = *selected;
      completion = &*selected;
      category = Language::Types::Composite::Category::Type;
      break;
    }
    case Tag::Interface: {
      auto selected = read_interface(reader, arena, composite);
      BAIL_IF(!selected);
      restored = *selected;
      completion = &*selected;
      category = Language::Types::Composite::Category::Type;
      break;
    }
    case Tag::Namespace: {
      auto selected = read_namespace(reader, arena, composite);
      BAIL_IF(!selected);
      restored = *selected;
      completion = &*selected;
      category = Language::Types::Composite::Category::Type;
      break;
    }
    case Tag::Object: {
      auto selected = read_object(reader, arena, composite);
      BAIL_IF(!selected);
      restored = *selected;
      completion = &*selected;
      category = Language::Types::Composite::Category::Type;
      break;
    }
    case Tag::Implemented: {
      auto selected = read_implemented(reader, arena, composite);
      BAIL_IF(!selected);
      restored = *selected;
      completion = &*selected;
      category = Language::Types::Composite::Category::Type;
      break;
    }
    case Tag::Enumeration: {
      auto selected = read_enumeration(reader, arena, composite);
      BAIL_IF(!selected);
      restored = *selected;
      completion = &*selected;
      category = Language::Types::Composite::Category::Type;
      break;
    }
    default:
      return False;
    }

    BAIL_IF(
        !restored ||
        !composite.retain_definition(
            *restored, category, is_published(*restored), completion));
  }
  return True;
}

auto Archive::write(Writer& writer, const Language::Types::Structure& structure)
    -> Bool {
  auto record = writer.begin(Tag::Structure);
  Declaration declaration(structure.get_definition());
  return declaration.write(writer) && write_declarations(writer, structure) &&
         writer.finish(record);
}

auto Archive::write(Writer& writer, const Language::Types::Namespace& selected)
    -> Bool {
  auto record = writer.begin(Tag::Namespace);
  Declaration declaration(selected.get_definition());
  return declaration.write(writer) && write_declarations(writer, selected) &&
         writer.finish(record);
}

auto Archive::read_namespace(
    Reader& reader,
    Allocator::Arena& arena,
    Abstract& host) -> Option<Language::Types::Namespace&> {
  auto record = reader.read_record();
  BAIL_IF(
      !record || record->get_tag() != U16(Tag::Namespace) ||
      record->is_optional());
  Reader contents(record->get_payload());
  auto declaration = Declaration::read(contents, arena);
  BAIL_IF(!declaration);
  auto& definition = declaration->create_definition(arena, host);
  auto& selected =
      Language::Types::Namespace::create_restored(arena, definition);
  BAIL_IF(!read_declarations(contents, arena, selected));
  selected.complete_body();
  return selected;
}

auto Archive::read_structure(
    Reader& reader,
    Allocator::Arena& arena,
    Abstract& host) -> Option<Language::Types::Structure&> {
  auto record = reader.read_record();
  BAIL_IF(
      !record || record->get_tag() != U16(Tag::Structure) ||
      record->is_optional());

  Reader contents(record->get_payload());
  auto declaration = Declaration::read(contents, arena);
  BAIL_IF(!declaration);
  auto& definition = declaration->create_definition(arena, host);
  auto& structure =
      Language::Types::Structure::create_restored(arena, definition);
  BAIL_IF(!read_declarations(contents, arena, structure));
  structure.complete_body();
  return structure;
}

auto Archive::write(Writer& writer, const Language::Types::Interface& interface)
    -> Bool {
  auto record = writer.begin(Tag::Interface);
  Declaration declaration(interface.get_definition());
  return declaration.write(writer) && write_declarations(writer, interface) &&
         writer.finish(record);
}

auto Archive::read_interface(
    Reader& reader,
    Allocator::Arena& arena,
    Abstract& host) -> Option<Language::Types::Interface&> {
  auto record = reader.read_record();
  BAIL_IF(
      !record || record->get_tag() != U16(Tag::Interface) ||
      record->is_optional());

  Reader contents(record->get_payload());
  auto declaration = Declaration::read(contents, arena);
  BAIL_IF(!declaration);
  auto& definition = declaration->create_definition(arena, host);
  auto& interface =
      Language::Types::Interface::create_restored(arena, definition);
  BAIL_IF(!read_declarations(contents, arena, interface));
  interface.complete_body();
  return interface;
}

auto Archive::write(Writer& writer, const Language::Types::Object& object)
    -> Bool {
  auto record = writer.begin(Tag::Object);
  Declaration declaration(object.get_definition());
  return declaration.write(writer) && write_declarations(writer, object) &&
         writer.finish(record);
}

auto Archive::read_object(
    Reader& reader,
    Allocator::Arena& arena,
    Abstract& host) -> Option<Language::Types::Object&> {
  auto record = reader.read_record();
  BAIL_IF(
      !record || record->get_tag() != U16(Tag::Object) ||
      record->is_optional());

  Reader contents(record->get_payload());
  auto declaration = Declaration::read(contents, arena);
  BAIL_IF(!declaration);
  auto& definition = declaration->create_definition(arena, host);
  auto& object = Language::Types::Object::create_restored(arena, definition);
  BAIL_IF(!read_declarations(contents, arena, object));
  object.complete_body();
  return object;
}

auto Archive::write(
    Writer& writer,
    const Language::Types::Implemented& implemented) -> Bool {
  auto record = writer.begin(Tag::Implemented);
  Declaration declaration(implemented.get_definition());
  return declaration.write(writer) &&
         Archive::write(writer, implemented.get_requirement_reference()) &&
         write_declarations(writer, implemented) && writer.finish(record);
}

auto Archive::read_implemented(
    Reader& reader,
    Allocator::Arena& arena,
    Abstract& host) -> Option<Language::Types::Implemented&> {
  auto record = reader.read_record();
  BAIL_IF(
      !record || record->get_tag() != U16(Tag::Implemented) ||
      record->is_optional());

  Reader contents(record->get_payload());
  auto declaration = Declaration::read(contents, arena);
  auto requirement = read_type_reference(contents, arena, host);
  BAIL_IF(!declaration || !requirement);
  auto& definition = declaration->create_definition(arena, host);
  auto& implemented = Language::Types::Implemented::create_restored(
      arena, definition, *requirement);
  BAIL_IF(!read_declarations(contents, arena, implemented));
  implemented.complete_body();
  return implemented;
}

auto Archive::write(
    Writer& writer,
    const Language::Types::Enumeration& enumeration) -> Bool {
  auto record = writer.begin(Tag::Enumeration);
  Declaration declaration(enumeration.get_definition());
  BAIL_IF(
      !declaration.write(writer) ||
      !Archive::write(writer, enumeration.get_storage_reference()) ||
      enumeration.get_case_count() > U32(-1));

  writer.write(U32(enumeration.get_case_count()));
  auto cases = enumeration.get_cases();
  for (Count index = 0; index < enumeration.get_case_count(); index++) {
    auto case_record = writer.begin(Tag::EnumerationCase);
    const Tetrodotoxin::Language::Binding& selected = *cases.get_data()[index];
    auto value = enumeration.get_case_value(index);
    BAIL_IF(
        !writer.write(selected.get_documentation()) ||
        !writer.write(selected.get_name()) || !value);
    writer.write(*value);
    BAIL_IF(!writer.finish(case_record));
  }
  return writer.finish(record);
}

auto Archive::read_enumeration(
    Reader& reader,
    Allocator::Arena& arena,
    Abstract& host) -> Option<Language::Types::Enumeration&> {
  auto record = reader.read_record();
  BAIL_IF(
      !record || record->get_tag() != U16(Tag::Enumeration) ||
      record->is_optional());

  Reader contents(record->get_payload());
  auto declaration = Declaration::read(contents, arena);
  auto storage = read_type_reference(contents, arena, host);
  auto count = contents.read_u32();
  BAIL_IF(
      !declaration || !storage || !count ||
      Count(*count) > contents.get_remaining_size() / 8);

  auto& definition = declaration->create_definition(arena, host);
  auto& enumeration = Language::Types::Enumeration::create(
      arena, definition, *storage,
      View::Vector<Language::Types::Enumeration::Case>());
  for (Count index = 0; index < *count; index++) {
    auto case_record = contents.read_record();
    BAIL_IF(
        !case_record || case_record->get_tag() != U16(Tag::EnumerationCase) ||
        case_record->is_optional());
    Reader case_contents(case_record->get_payload());
    auto documentation = case_contents.read_documentation(arena);
    auto name = case_contents.read_bytes();
    auto value = case_contents.read_u64();
    BAIL_IF(
        !documentation || !name || name->is_empty() || !value ||
        !case_contents.is_complete() ||
        !enumeration.retain_restored_case(
            arena.proxy(*name), *value, *documentation));
  }
  BAIL_IF(!contents.is_complete());
  return enumeration;
}
