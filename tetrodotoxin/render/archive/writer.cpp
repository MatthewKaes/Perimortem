// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/render/archive/writer.hpp"

#include "perimortem/core/writer/binary.hpp"

#include "perimortem/serialization/stream/binary.hpp"

#include "tetrodotoxin/render/language/alias.hpp"
#include "tetrodotoxin/render/language/binding.hpp"
#include "tetrodotoxin/render/language/stage.hpp"
#include "tetrodotoxin/render/language/structure.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Perimortem::Serialization;
using namespace Ttx::Concept;
using namespace Tetrodotoxin;

using Appender = Stream::Binary<Data::ByteOrder::Little, Dynamic::Bytes>;
using Patcher = Perimortem::Core::Writer::Binary<Data::ByteOrder::Little>;

enum class RenderWriterAttributeValue : U8 {
  Empty,
  Bytes,
  Unsigned,
  Signed,
  Real,
  Flag,
};

static auto definition_of(const Abstract& declaration)
    -> Option<const Tetrodotoxin::Language::Definition&> {
  auto alias = declaration.select<Render::Language::Alias>();
  if (alias) {
    return alias->get_definition();
  }
  auto binding = declaration.select<Render::Language::Binding>();
  if (binding) {
    return binding->get_definition();
  }
  auto stage = declaration.select<Render::Language::Stage>();
  if (stage) {
    return stage->get_definition();
  }
  auto structure = declaration.select<Render::Language::Structure>();
  return structure ? structure->get_definition()
                   : Option<const Tetrodotoxin::Language::Definition&>();
}

Render::Archive::Writer::Writer(
    Tetrodotoxin::Language::Persistence::Profile profile)
    : profile(profile) {
  Appender appender(bytes);
  appender << "TTXR"_view;
  appender << U16(1);
  appender << U8(profile);
  appender << U8(0);
}

auto Render::Archive::Writer::encode(
    const Render::Language::Monograph& monograph,
    Tetrodotoxin::Language::Persistence::Profile profile)
    -> Option<Dynamic::Bytes> {
  BAIL_IF(!monograph.is_finalized());

  Writer writer(profile);
  BAIL_IF(!writer.write(monograph));
  return Data::take(writer.bytes);
}

auto Render::Archive::Writer::begin(Tag tag) -> Record {
  Count offset = bytes.get_size();
  Appender appender(bytes);
  appender << U16(tag);
  appender << U16(0);
  appender << U32(0);
  return Record(offset);
}

auto Render::Archive::Writer::finish(Record record) -> Bool {
  Count offset = record.get_offset();
  BAIL_IF(offset > bytes.get_size() || bytes.get_size() - offset < 8);

  Count size = bytes.get_size() - offset - 8;
  BAIL_IF(size > U32(-1));
  Patcher patcher(bytes.get_access().slice(offset + 4, 4));
  patcher << U32(size);
  return patcher.is_valid();
}

auto Render::Archive::Writer::write(U8 value) -> void {
  Appender(bytes) << value;
}

auto Render::Archive::Writer::write(U32 value) -> void {
  Appender(bytes) << value;
}

auto Render::Archive::Writer::write(U64 value) -> void {
  Appender(bytes) << value;
}

auto Render::Archive::Writer::write(S64 value) -> void {
  Appender(bytes) << value;
}

auto Render::Archive::Writer::write(R64 value) -> void {
  Appender(bytes) << value;
}

auto Render::Archive::Writer::write(View::Bytes value) -> Bool {
  BAIL_IF(value.get_size() > U32(-1));
  Appender appender(bytes);
  appender << U32(value.get_size());
  appender << value;
  return True;
}

auto Render::Archive::Writer::write(const Documentation& value) -> Bool {
  BAIL_IF(value.line_count() > U32(-1));
  write(U32(value.line_count()));
  for (Count index = 0; index < value.line_count(); index++) {
    BAIL_IF(!write(value.get_line(index)));
  }
  return True;
}

auto Render::Archive::Writer::write(
    View::Vector<Tetrodotoxin::Language::Attribute> attributes) -> Bool {
  BAIL_IF(attributes.get_size() > U32(-1));
  write(U32(attributes.get_size()));
  for (const Tetrodotoxin::Language::Attribute& attribute : attributes) {
    BAIL_IF(!write(attribute.get_key()));
    const auto& value = attribute.get_value();
    Bool written = value.visit(
        [&]() -> Bool {
          write(U8(RenderWriterAttributeValue::Empty));
          return True;
        },
        [&](View::Bytes selected) -> Bool {
          write(U8(RenderWriterAttributeValue::Bytes));
          return write(selected);
        },
        [&](U64 selected) -> Bool {
          write(U8(RenderWriterAttributeValue::Unsigned));
          write(selected);
          return True;
        },
        [&](S64 selected) -> Bool {
          write(U8(RenderWriterAttributeValue::Signed));
          write(selected);
          return True;
        },
        [&](R64 selected) -> Bool {
          write(U8(RenderWriterAttributeValue::Real));
          write(selected);
          return True;
        },
        [&](Bool selected) -> Bool {
          write(U8(RenderWriterAttributeValue::Flag));
          write(U8(selected ? 1 : 0));
          return True;
        });
    BAIL_IF(!written);
  }
  return True;
}

auto Render::Archive::Writer::write(
    const Tetrodotoxin::Language::Definition& definition) -> Bool {
  BAIL_IF(
      !write(definition.get_documentation()) ||
      !write(definition.get_attributes()) || !write(definition.get_name()));
  write(U8(definition.get_visibility()));
  return True;
}

auto Render::Archive::Writer::write(
    const Tetrodotoxin::Language::TypeReference& reference) -> Bool {
  return write(reference.get_route());
}

auto Render::Archive::Writer::write(const Render::Language::Layout& layout)
    -> Bool {
  auto record = begin(Tag::Layout);
  auto slots = layout.get_slots();
  BAIL_IF(slots.get_size() > U32(-1));
  write(U8(layout.contains_parameters() ? 1 : 0));
  write(U32(slots.get_size()));
  for (const Render::Language::Layout::Slot& slot : slots) {
    auto slot_record = begin(Tag::Slot);
    BAIL_IF(
        !write(slot.get_name()) || !write(slot.get_attributes()) ||
        !write(slot.get_type()) || !finish(slot_record));
  }
  return finish(record);
}

auto Render::Archive::Writer::write(const Abstract& declaration) -> Bool {
  auto definition = definition_of(declaration);
  BAIL_IF(!definition || (public_only() && !definition->is_published()));

  auto alias = declaration.select<Render::Language::Alias>();
  if (alias) {
    auto record = begin(Tag::Alias);
    return write(alias->get_definition()) &&
           write(alias->get_target_reference()) && finish(record);
  }

  auto binding = declaration.select<Render::Language::Binding>();
  if (binding) {
    auto reference = binding->get_type_reference();
    auto binding_definition = binding->get_definition();
    BAIL_IF(
        !reference || !binding_definition ||
        binding->get_kind() == Render::Language::Binding::Kind::Parameter);
    auto record = begin(Tag::Binding);
    BAIL_IF(!write(*binding_definition));
    write(U8(binding->get_kind()));
    return write(*reference) && finish(record);
  }

  auto stage = declaration.select<Render::Language::Stage>();
  if (stage) {
    auto record = begin(Tag::Stage);
    return write(stage->get_definition()) &&
           write(stage->get_parameter_layout()) &&
           write(stage->get_result_layout()) && finish(record);
  }

  auto structure = declaration.select<Render::Language::Structure>();
  return structure && write(*structure);
}

auto Render::Archive::Writer::write(
    const Render::Language::Structure& structure) -> Bool {
  // Declaration categories retain their separate lookup meaning. Value
  // Bindings stay in addressable order, which also preserves instance Layout.
  auto record = begin(Tag::Structure);
  BAIL_IF(!write(structure.get_definition()));

  for (const Reference<Abstract>& declaration : structure.get_types()) {
    auto definition = definition_of(declaration.get());
    if (!public_only() || (definition && definition->is_published())) {
      BAIL_IF(!write(declaration.get()));
    }
  }
  for (const Reference<Abstract>& declaration : structure.get_addressables()) {
    auto definition = definition_of(declaration.get());
    if (!public_only() || (definition && definition->is_published())) {
      BAIL_IF(!write(declaration.get()));
      continue;
    }

    auto binding = declaration.get().select<Render::Language::Binding>();
    if (binding &&
        binding->get_kind() == Render::Language::Binding::Kind::Value) {
      // Public consumers still need the complete Layout even though the
      // private spelling and Binding are outside the Contract query surface.
      auto reference = binding->get_type_reference();
      BAIL_IF(!reference);
      auto hidden = begin(Tag::HiddenSlot);
      BAIL_IF(!write(*reference) || !finish(hidden));
    }
  }
  for (const Reference<Abstract>& declaration : structure.get_callables()) {
    auto definition = definition_of(declaration.get());
    if (!public_only() || (definition && definition->is_published())) {
      BAIL_IF(!write(declaration.get()));
    }
  }

  return finish(record);
}

auto Render::Archive::Writer::write(
    const Render::Language::Monograph& monograph) -> Bool {
  auto record = begin(Tag::Monograph);
  BAIL_IF(!write(monograph.get_documentation()));
  for (const Reference<Abstract>& declaration : monograph.get_types()) {
    auto definition = definition_of(declaration.get());
    if (!public_only() || (definition && definition->is_published())) {
      BAIL_IF(!write(declaration.get()));
    }
  }
  for (const Reference<Abstract>& declaration : monograph.get_addressables()) {
    auto definition = definition_of(declaration.get());
    if (!public_only() || (definition && definition->is_published())) {
      BAIL_IF(!write(declaration.get()));
    }
  }
  for (const Reference<Abstract>& declaration : monograph.get_callables()) {
    auto definition = definition_of(declaration.get());
    if (!public_only() || (definition && definition->is_published())) {
      BAIL_IF(!write(declaration.get()));
    }
  }
  return finish(record);
}
