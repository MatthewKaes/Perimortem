// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/library/archive/reference.hpp"

#include "perimortem/memory/managed/vector.hpp"

#include "tetrodotoxin/library/language/constants/false.hpp"
#include "tetrodotoxin/library/language/constants/signed.hpp"
#include "tetrodotoxin/library/language/constants/true.hpp"
#include "tetrodotoxin/library/language/constants/unsigned.hpp"
#include "tetrodotoxin/library/language/model/types/flag.hpp"
#include "tetrodotoxin/library/language/model/types/signed.hpp"
#include "tetrodotoxin/library/language/model/types/unsigned.hpp"

using namespace Perimortem;
using namespace Ttx::Concept;
using namespace Ttx::Lexical;
using namespace Tetrodotoxin::Library;

enum class PersistedArgument : U8 {
  Reference,
  Unsigned,
  Signed,
  False,
  True,
};

static auto resolve_root_type(const Abstract& context, Core::View::Bytes name)
    -> Core::Option<const Language::Model::Type&> {
  return context.resolve_concept(name)
      .resolve()
      .select<Language::Model::Type>();
}

static auto write_argument(
    Archive::Writer& writer,
    const Language::TypeReference::Argument& argument) -> Bool {
  return argument.visit(
      []() -> Bool { return False; },
      [&](const Language::TypeReference& reference) -> Bool {
        writer.write(U8(PersistedArgument::Reference));
        return Archive::write(writer, reference);
      },
      [&](const Abstract& selected) -> Bool {
        auto unsigned_value = selected.select<Language::Constants::Unsigned>();
        if (unsigned_value) {
          writer.write(U8(PersistedArgument::Unsigned));
          writer.write(unsigned_value->get_value());
          return True;
        }

        auto signed_value = selected.select<Language::Constants::Signed>();
        if (signed_value) {
          writer.write(U8(PersistedArgument::Signed));
          writer.write(signed_value->get_value());
          return True;
        }

        if (selected.is<Language::Constants::False>()) {
          writer.write(U8(PersistedArgument::False));
          return True;
        }
        if (selected.is<Language::Constants::True>()) {
          writer.write(U8(PersistedArgument::True));
          return True;
        }
        return False;
      });
}

static auto read_argument(
    Archive::Reader& reader,
    Memory::Allocator::Arena& arena,
    const Abstract& context)
    -> Core::Option<Language::TypeReference::Argument> {
  auto kind = reader.read_u8();
  BAIL_IF(!kind);

  switch (PersistedArgument(*kind)) {
  case PersistedArgument::Reference: {
    auto reference = Archive::read_type_reference(reader, arena, context);
    BAIL_IF(!reference);
    const auto& retained = arena.construct<Language::TypeReference>(*reference);
    return Language::TypeReference::Argument(retained);
  }
  case PersistedArgument::Unsigned: {
    auto value = reader.read_u64();
    auto type = resolve_root_type(context, "U64"_view);
    auto selected =
        type ? type->select<Language::Model::Types::Unsigned>()
             : Core::Option<const Language::Model::Types::Unsigned&>();
    BAIL_IF(!value || !selected);
    const auto& constant = Language::Constants::Unsigned::create_synthetic(
        arena, *selected, *value);
    return Language::TypeReference::Argument(
        static_cast<const Abstract&>(constant));
  }
  case PersistedArgument::Signed: {
    auto value = reader.read_s64();
    auto type = resolve_root_type(context, "S64"_view);
    auto selected = type
                        ? type->select<Language::Model::Types::Signed>()
                        : Core::Option<const Language::Model::Types::Signed&>();
    BAIL_IF(!value || !selected);
    const auto& constant =
        Language::Constants::Signed::create_synthetic(arena, *selected, *value);
    return Language::TypeReference::Argument(
        static_cast<const Abstract&>(constant));
  }
  case PersistedArgument::False:
  case PersistedArgument::True: {
    auto type = resolve_root_type(context, "Bool"_view);
    auto selected = type ? type->select<Language::Model::Types::Flag>()
                         : Core::Option<const Language::Model::Types::Flag&>();
    BAIL_IF(!selected);
    const Abstract& constant =
        PersistedArgument(*kind) == PersistedArgument::True
            ? static_cast<const Abstract&>(
                  Language::Constants::True::create_synthetic(arena, *selected))
            : static_cast<const Abstract&>(
                  Language::Constants::False::create_synthetic(
                      arena, *selected));
    return Language::TypeReference::Argument(constant);
  }
  }

  return {};
}

auto Archive::write(Writer& writer, const Language::TypeReference& reference)
    -> Bool {
  auto record = writer.begin(Tag::TypeReference);
  BAIL_IF(
      !writer.write(reference.get_route()) ||
      reference.get_argument_size() > U32(-1));

  writer.write(U32(reference.get_argument_size()));
  for (Count index = 0; index < reference.get_argument_size(); index++) {
    auto argument = reference.get_argument(index);
    BAIL_IF(!argument || !write_argument(writer, *argument));
  }
  return writer.finish(record);
}

auto Archive::read_type_reference(
    Reader& reader,
    Memory::Allocator::Arena& arena,
    const Abstract& context) -> Core::Option<Language::TypeReference> {
  auto record = reader.read_record();
  BAIL_IF(
      !record || record->get_tag() != U16(Tag::TypeReference) ||
      record->is_optional());

  Reader contents(record->get_payload());
  auto route = contents.read_bytes();
  auto count = contents.read_u32();
  BAIL_IF(!route || route->is_empty() || !count);

  Memory::Managed::Vector<Language::TypeReference::Argument> restored(arena);
  for (Count index = 0; index < *count; index++) {
    auto argument = read_argument(contents, arena, context);
    BAIL_IF(!argument);
    restored.insert(*argument);
  }
  BAIL_IF(!contents.is_complete());

  Core::Option<Core::View::Vector<Language::TypeReference::Argument>> arguments;
  if (*count != 0) {
    arguments = restored.get_view();
  }
  return Language::TypeReference::create(
      arena.proxy(*route), Anchor::create(Span()), Token(), arguments);
}

auto Archive::write(Writer& writer, const Language::Model::Layout& layout)
    -> Bool {
  auto record = writer.begin(Tag::Layout);
  BAIL_IF(layout.get_size() > U32(-1));

  writer.write(U32(layout.get_size()));
  for (Count index = 0; index < layout.get_size(); index++) {
    BAIL_IF(!writer.write(layout.get_declared_name(index)));

    auto attributes = layout.get_slot_attributes(index);
    BAIL_IF(attributes.get_size() > U32(-1));
    writer.write(U32(attributes.get_size()));
    for (Count attribute_index = 0; attribute_index < attributes.get_size();
         attribute_index++) {
      BAIL_IF(!writer.write(attributes.get_data()[attribute_index]));
    }

    auto reference = layout.get_type_reference(index);
    writer.write(U8(reference ? 1 : 0));
    BAIL_IF(reference && !Archive::write(writer, *reference));
  }
  return writer.finish(record);
}

auto Archive::read_layout(
    Reader& reader,
    Memory::Allocator::Arena& arena,
    const Abstract& context,
    Bool parameters) -> Core::Option<Language::Model::Layout&> {
  auto record = reader.read_record();
  BAIL_IF(
      !record || record->get_tag() != U16(Tag::Layout) ||
      record->is_optional());

  Reader contents(record->get_payload());
  auto count = contents.read_u32();
  BAIL_IF(!count || Count(*count) > record->get_payload().get_size());

  Memory::Managed::Vector<Language::Model::Layout::Slot> slots(arena);
  for (Count index = 0; index < *count; index++) {
    auto name = contents.read_bytes();
    auto attribute_count = contents.read_u32();
    BAIL_IF(!name || !attribute_count);
    Memory::Managed::Vector<Tetrodotoxin::Language::Attribute> attributes(
        arena);
    for (Count attribute_index = 0; attribute_index < *attribute_count;
         attribute_index++) {
      auto attribute = contents.read_attribute(arena);
      BAIL_IF(!attribute);
      attributes.insert(*attribute);
    }
    auto has_reference = contents.read_u8();
    BAIL_IF(!has_reference || *has_reference > 1);

    Core::Option<Language::TypeReference> reference;
    if (*has_reference == 1) {
      auto restored = read_type_reference(contents, arena, context);
      BAIL_IF(!restored);
      reference = *restored;
    }

    slots.insert(
        Language::Model::Layout::Slot(
            reference, Anchor::create(Span()), arena.proxy(*name),
            attributes.get_view()));
  }
  BAIL_IF(!contents.is_complete());
  return Language::Model::Layout::create(
      arena, slots, Anchor::create(Span()), parameters);
}

auto Archive::write(Writer& writer, const Language::Signature& signature)
    -> Bool {
  auto record = writer.begin(Tag::Signature);
  BAIL_IF(
      !Archive::write(writer, signature.get_parameters()) ||
      !Archive::write(writer, signature.get_results()) ||
      !writer.finish(record));
  return True;
}

auto Archive::read_signature(
    Reader& reader,
    Memory::Allocator::Arena& arena,
    const Abstract& host) -> Core::Option<Language::Signature&> {
  auto record = reader.read_record();
  BAIL_IF(
      !record || record->get_tag() != U16(Tag::Signature) ||
      record->is_optional());

  Reader contents(record->get_payload());
  auto parameters = read_layout(contents, arena, host, True);
  auto results = read_layout(contents, arena, host, False);
  BAIL_IF(!parameters || !results || !contents.is_complete());
  return Language::Signature::create(arena, host, *parameters, *results);
}
