// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/library/archive/composite.hpp"

#include "tetrodotoxin/library/archive/declaration.hpp"
#include "tetrodotoxin/library/archive/reference.hpp"
#include "tetrodotoxin/library/language/alias.hpp"
#include "tetrodotoxin/library/language/field.hpp"
#include "tetrodotoxin/library/language/function.hpp"
#include "ttx/concept/reference.hpp"

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
    const Language::Types::Composite& composite,
    Bool public_only) -> Bool {
  Count hidden_slot = 0;
  for (const Reference<Abstract>& retained : composite.get_declarations()) {
    const Abstract& declaration = retained.get();
    if (public_only && !composite.is_published(declaration)) {
      auto field = declaration.select<Language::Field>();
      if (field && field->contributes_to_instance_layout()) {
        BAIL_IF(!write_field_slot(writer, *field, hidden_slot));
        hidden_slot++;
      }
      continue;
    }

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
    auto structure = declaration.select<Language::Types::Structure>();
    auto object = declaration.select<Language::Types::Object>();
    if (object) {
      BAIL_IF(!Archive::write(writer, *object));
      continue;
    }
    if (structure) {
      BAIL_IF(!Archive::write(writer, *structure));
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
    Language::Types::Composite& composite,
    Tetrodotoxin::Language::Persistence::Profile profile) -> Bool {
  using Category = Language::Types::Composite::Category;
  Count hidden_slot = 0;
  while (!reader.is_complete()) {
    Reader probe = reader;
    auto record = probe.read_record();
    BAIL_IF(!record);

    if (record->is_optional()) {
      BAIL_IF(!reader.read_record());
      continue;
    }

    Option<Abstract&> restored;
    Category category = Category::Addressable;
    switch (Tag(record->get_tag())) {
    case Tag::Alias: {
      auto selected = read_alias(reader, arena, composite);
      BAIL_IF(!selected);
      restored = *selected;
      category = Category::Type;
      break;
    }
    case Tag::Field: {
      auto selected = read_field(reader, arena, composite);
      BAIL_IF(!selected);
      restored = *selected;
      break;
    }
    case Tag::FieldSlot: {
      BAIL_IF(
          profile != Tetrodotoxin::Language::Persistence::Profile::Contract);
      auto selected = read_field_slot(reader, arena, composite, hidden_slot);
      BAIL_IF(!selected);
      restored = *selected;
      hidden_slot++;
      break;
    }
    case Tag::Function: {
      auto selected = read_function(reader, arena, composite);
      BAIL_IF(!selected);
      restored = *selected;
      category = Category::Callable;
      break;
    }
    case Tag::Structure: {
      auto selected = read_structure(reader, arena, composite, profile);
      BAIL_IF(!selected);
      restored = *selected;
      category = Category::Type;
      break;
    }
    case Tag::Object: {
      auto selected = read_object(reader, arena, composite, profile);
      BAIL_IF(!selected);
      restored = *selected;
      category = Category::Type;
      break;
    }
    case Tag::Enumeration: {
      auto selected = read_enumeration(reader, arena, composite);
      BAIL_IF(!selected);
      restored = *selected;
      category = Category::Type;
      break;
    }
    default:
      return False;
    }

    BAIL_IF(
        !restored || !composite.retain_definition(
                         *restored, category, is_published(*restored)));
  }
  return True;
}

auto Archive::write(Writer& writer, const Language::Types::Structure& structure)
    -> Bool {
  auto record = writer.begin(Tag::Structure);
  Declaration declaration(structure.get_definition());
  Bool public_only = writer.get_profile() ==
                     Tetrodotoxin::Language::Persistence::Profile::Contract;
  return declaration.write(writer) &&
         write_declarations(writer, structure, public_only) &&
         writer.finish(record);
}

auto Archive::read_structure(
    Reader& reader,
    Allocator::Arena& arena,
    Abstract& host,
    Tetrodotoxin::Language::Persistence::Profile profile)
    -> Option<Language::Types::Structure&> {
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
  BAIL_IF(!read_declarations(contents, arena, structure, profile));
  structure.complete_body();
  return structure;
}

auto Archive::write(Writer& writer, const Language::Types::Object& object)
    -> Bool {
  auto record = writer.begin(Tag::Object);
  Declaration declaration(object.get_definition());
  Bool public_only = writer.get_profile() ==
                     Tetrodotoxin::Language::Persistence::Profile::Contract;
  return declaration.write(writer) &&
         write_declarations(writer, object, public_only) &&
         writer.finish(record);
}

auto Archive::read_object(
    Reader& reader,
    Allocator::Arena& arena,
    Abstract& host,
    Tetrodotoxin::Language::Persistence::Profile profile)
    -> Option<Language::Types::Object&> {
  auto record = reader.read_record();
  BAIL_IF(
      !record || record->get_tag() != U16(Tag::Object) ||
      record->is_optional());

  Reader contents(record->get_payload());
  auto declaration = Declaration::read(contents, arena);
  BAIL_IF(!declaration);
  auto& definition = declaration->create_definition(arena, host);
  auto& object = Language::Types::Object::create_restored(arena, definition);
  BAIL_IF(!read_declarations(contents, arena, object, profile));
  object.complete_body();
  return object;
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
    const Ttx::Model::Alias& selected = cases.get_data()[index].get();
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
