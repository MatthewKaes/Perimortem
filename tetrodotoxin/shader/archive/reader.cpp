// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/shader/archive/reader.hpp"

#include "perimortem/core/reader/binary.hpp"

#include "perimortem/memory/managed/vector.hpp"

#include "tetrodotoxin/library/archive/reader.hpp"
#include "tetrodotoxin/library/language/field.hpp"
#include "tetrodotoxin/render/language/attributes.hpp"
#include "ttx/bootstrap/concept/unknown.hpp"
#include "ttx/bootstrap/model/documentations/block.hpp"
#include "ttx/lexical/anchor.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Ttx::Concept;
using namespace Tetrodotoxin;

enum class ShaderReaderAttributeValue : U8 {
  Empty,
  Bytes,
  Unsigned,
  Signed,
  Real,
  Flag,
};

static auto find_attribute(
    View::Vector<Tetrodotoxin::Language::Attribute> attributes,
    View::Bytes key) -> Option<const Tetrodotoxin::Language::Attribute&> {
  for (Count index = 0; index < attributes.get_size(); index++) {
    const Tetrodotoxin::Language::Attribute& attribute =
        attributes.get_data()[index];
    if (attribute.get_key() == key) {
      return attribute;
    }
  }
  return {};
}

static auto bridge_attributes_match(
    View::Vector<Tetrodotoxin::Language::Attribute> attributes,
    Shader::Language::Bridge::Direction direction,
    Shader::Language::Bridge::Marshaling marshaling,
    Shader::Language::Bridge::Synchronization synchronization) -> Bool {
  BAIL_IF(attributes.get_size() != 3);
  auto direction_attribute = find_attribute(attributes, "direction"_view);
  auto marshaling_attribute = find_attribute(attributes, "marshal"_view);
  auto synchronization_attribute = find_attribute(attributes, "sync"_view);
  BAIL_IF(
      !direction_attribute || !marshaling_attribute ||
      !synchronization_attribute);
  const View::Bytes* direction_name =
      direction_attribute->get_value().find<View::Bytes>();
  const View::Bytes* marshaling_name =
      marshaling_attribute->get_value().find<View::Bytes>();
  const View::Bytes* synchronization_name =
      synchronization_attribute->get_value().find<View::Bytes>();
  BAIL_IF(!direction_name || !marshaling_name || !synchronization_name);

  View::Bytes expected_direction =
      direction == Shader::Language::Bridge::Direction::Upload ? "upload"_view
      : direction == Shader::Language::Bridge::Direction::Download
          ? "download"_view
          : "bidirectional"_view;
  View::Bytes expected_marshaling =
      marshaling == Shader::Language::Bridge::Marshaling::Identity
          ? "identity"_view
      : marshaling == Shader::Language::Bridge::Marshaling::Copy ? "copy"_view
                                                                 : "pack"_view;
  View::Bytes expected_synchronization =
      synchronization == Shader::Language::Bridge::Synchronization::None
          ? "none"_view
      : synchronization == Shader::Language::Bridge::Synchronization::Submission
          ? "submission"_view
          : "frame"_view;
  return *direction_name == expected_direction &&
         *marshaling_name == expected_marshaling &&
         *synchronization_name == expected_synchronization;
}

auto Shader::Archive::Reader::Definition::create(
    Allocator::Arena& arena,
    Abstract& host) const -> Tetrodotoxin::Language::Definition& {
  return Tetrodotoxin::Language::Definition::create_restored(
      arena, documentation, host, attributes, name, visibility);
}

auto Shader::Archive::Reader::open(View::Bytes payload) -> Option<Reader> {
  BAIL_IF(payload.get_size() < 8);
  Perimortem::Core::Reader::Binary<Data::ByteOrder::Little> reader(
      payload.slice(0, 8));
  View::Bytes magic = reader.read_bytes(4);
  U16 version = reader.read_u16();
  U16 flags = reader.read_u16();
  BAIL_IF(magic != "TTXS"_view || version != 2 || flags != 0);
  return Reader(payload.slice(8));
}

auto Shader::Archive::Reader::restore(
    Allocator::Arena& arena,
    View::Bytes payload,
    const Abstract& language,
    const Library::Dialect& library,
    Abstract& context) -> Option<Shader::Language::Monograph&> {
  // Shader and its Library child share one reconstruction Arena just as they
  // share one authored source transaction. Program records can then restore
  // Library declarations into the exact Shader subtype they describe.
  auto opened = open(payload);
  BAIL_IF(!opened);
  auto record = opened->read_record();
  BAIL_IF(
      !record || record->get_tag() != U16(Tag::Monograph) ||
      !opened->is_complete());

  Reader contents(record->get_payload());
  auto documentation = contents.read_documentation(arena);
  BAIL_IF(!documentation);
  auto& child = Library::Language::Monograph::create(
      arena, *documentation, Ttx::Lexical::Anchor::create(Ttx::Lexical::Span()),
      library, context);
  auto& monograph = Shader::Language::Monograph::create(
      arena, language, *documentation, context, child);
  while (!contents.is_complete()) {
    Reader probe = contents;
    auto next = probe.read_record();
    BAIL_IF(!next);
    if (next->get_tag() == U16(Tag::Program)) {
      BAIL_IF(!contents.read_program(arena, monograph));
    } else if (next->get_tag() == U16(Tag::Bridge)) {
      BAIL_IF(!contents.read_bridge(arena, monograph));
    } else {
      return {};
    }
  }
  return monograph;
}

auto Shader::Archive::Reader::take(Count size) -> Option<View::Bytes> {
  BAIL_IF(
      location > payload.get_size() || size > payload.get_size() - location);
  View::Bytes selected = payload.slice(location, size);
  location += size;
  return selected;
}

auto Shader::Archive::Reader::read_record() -> Option<Record> {
  auto header = take(8);
  BAIL_IF(!header);
  Perimortem::Core::Reader::Binary<Data::ByteOrder::Little> reader(*header);
  U16 tag = reader.read_u16();
  U16 flags = reader.read_u16();
  U32 size = reader.read_u32();
  BAIL_IF(flags != 0);
  auto selected = take(size);
  BAIL_IF(!selected);
  return Record(tag, *selected);
}

auto Shader::Archive::Reader::read_u8() -> Option<U8> {
  auto selected = take(sizeof(U8));
  return selected
             ? Option<U8>(
                   Perimortem::Core::Reader::Binary<Data::ByteOrder::Little>(
                       *selected)
                       .read_u8())
             : Option<U8>();
}

auto Shader::Archive::Reader::read_u32() -> Option<U32> {
  auto selected = take(sizeof(U32));
  return selected
             ? Option<U32>(
                   Perimortem::Core::Reader::Binary<Data::ByteOrder::Little>(
                       *selected)
                       .read_u32())
             : Option<U32>();
}

auto Shader::Archive::Reader::read_u64() -> Option<U64> {
  auto selected = take(sizeof(U64));
  return selected
             ? Option<U64>(
                   Perimortem::Core::Reader::Binary<Data::ByteOrder::Little>(
                       *selected)
                       .read_u64())
             : Option<U64>();
}

auto Shader::Archive::Reader::read_s64() -> Option<S64> {
  auto selected = take(sizeof(S64));
  return selected
             ? Option<S64>(
                   Perimortem::Core::Reader::Binary<Data::ByteOrder::Little>(
                       *selected)
                       .read_s64())
             : Option<S64>();
}

auto Shader::Archive::Reader::read_r64() -> Option<R64> {
  auto selected = take(sizeof(R64));
  return selected
             ? Option<R64>(
                   Perimortem::Core::Reader::Binary<Data::ByteOrder::Little>(
                       *selected)
                       .read_r64())
             : Option<R64>();
}

auto Shader::Archive::Reader::read_bytes() -> Option<View::Bytes> {
  auto size = read_u32();
  BAIL_IF(!size);
  return take(*size);
}

auto Shader::Archive::Reader::read_documentation(Allocator::Arena& arena)
    -> Option<const Documentation&> {
  auto count = read_u32();
  BAIL_IF(!count || Count(*count) > payload.get_size());
  auto lines = arena.reserve<View::Bytes>(*count);
  for (Count index = 0; index < *count; index++) {
    auto line = read_bytes();
    BAIL_IF(!line);
    lines.get_data()[index] = arena.proxy(*line);
  }
  return arena.construct<Ttx::Model::Documentations::Block>(
      View::Vector<View::Bytes>(lines.get_data(), lines.get_size()));
}

auto Shader::Archive::Reader::read_attributes(Allocator::Arena& arena)
    -> Option<View::Vector<Tetrodotoxin::Language::Attribute>> {
  auto count = read_u32();
  BAIL_IF(!count || Count(*count) > payload.get_size());
  Managed::Vector<Tetrodotoxin::Language::Attribute> attributes(arena);
  for (Count index = 0; index < *count; index++) {
    auto key = read_bytes();
    auto kind = read_u8();
    BAIL_IF(!key || key->is_empty() || !kind);
    Tetrodotoxin::Language::Attribute::Value value;
    switch (ShaderReaderAttributeValue(*kind)) {
    case ShaderReaderAttributeValue::Empty:
      break;
    case ShaderReaderAttributeValue::Bytes: {
      auto selected = read_bytes();
      BAIL_IF(!selected);
      value = Tetrodotoxin::Language::Attribute::Value(arena.proxy(*selected));
      break;
    }
    case ShaderReaderAttributeValue::Unsigned: {
      auto selected = read_u64();
      BAIL_IF(!selected);
      value = Tetrodotoxin::Language::Attribute::Value(*selected);
      break;
    }
    case ShaderReaderAttributeValue::Signed: {
      auto selected = read_s64();
      BAIL_IF(!selected);
      value = Tetrodotoxin::Language::Attribute::Value(*selected);
      break;
    }
    case ShaderReaderAttributeValue::Real: {
      auto selected = read_r64();
      BAIL_IF(!selected);
      value = Tetrodotoxin::Language::Attribute::Value(*selected);
      break;
    }
    case ShaderReaderAttributeValue::Flag: {
      auto selected = read_u8();
      BAIL_IF(!selected || *selected > 1);
      value = Tetrodotoxin::Language::Attribute::Value(
          *selected == 1 ? True : False);
      break;
    }
    default:
      return {};
    }
    attributes.insert(
        Tetrodotoxin::Language::Attribute::create_synthetic(
            arena.proxy(*key), value));
  }
  return attributes.get_view();
}

auto Shader::Archive::Reader::read_definition(Allocator::Arena& arena)
    -> Option<Definition> {
  auto documentation = read_documentation(arena);
  auto attributes = read_attributes(arena);
  auto name = read_bytes();
  auto visibility = read_u8();
  BAIL_IF(
      !documentation || !attributes || !name || name->is_empty() ||
      !visibility ||
      *visibility > U8(Tetrodotoxin::Language::Visibility::Exposed));
  auto selected_visibility = Tetrodotoxin::Language::Visibility(*visibility);
  return Definition(
      *documentation, *attributes, arena.proxy(*name), selected_visibility);
}

auto Shader::Archive::Reader::read_program(
    Allocator::Arena& arena,
    Shader::Language::Monograph& monograph)
    -> Option<Shader::Language::Program&> {
  // Shader owns the Definition and Render route while Library owns the opaque
  // declaration payload. Constructing Program first preserves one identity for
  // both contracts instead of restoring an ordinary Structure beside it.
  auto record = read_record();
  BAIL_IF(!record || record->get_tag() != U16(Tag::Program));
  Reader contents(record->get_payload());
  auto definition = contents.read_definition(arena);
  auto contract = contents.read_bytes();
  auto declarations = contents.read_bytes();
  auto binding_count = contents.read_u32();
  BAIL_IF(
      !definition || !contract || contract->is_empty() || !declarations ||
      !binding_count ||
      !Render::Language::Attributes::accepts(
          definition->get_attributes(),
          Render::Language::Attributes::Placement::Structure));

  auto& restored_definition =
      definition->create(arena, monograph.edit_library().get_source());
  auto contract_reference = Tetrodotoxin::Language::TypeReference::create(
      arena.proxy(*contract),
      Ttx::Lexical::Anchor::create(Ttx::Lexical::Span()));
  auto& program = Shader::Language::Program::create_restored(
      arena, restored_definition, contract_reference, monograph);
  BAIL_IF(!Library::Archive::Reader::restore_declarations(
      arena, *declarations, program));
  program.complete_body();
  BAIL_IF(!program.restore_runtime_surface());
  BAIL_IF(
      !monograph.edit_library().get_source().retain_definition(
          program, Library::Language::Types::Composite::Category::Type,
          False) ||
      !monograph.retain_program(program));

  for (Count index = 0; index < *binding_count; index++) {
    // Binding records reconnect storage meaning to Fields already restored by
    // Library. Their names select existing identities without another
    // declaration inventory.
    auto binding_record = contents.read_record();
    BAIL_IF(!binding_record || binding_record->get_tag() != U16(Tag::Binding));
    Reader binding_contents(binding_record->get_payload());
    auto name = binding_contents.read_bytes();
    auto kind = binding_contents.read_u8();
    auto access = binding_contents.read_u8();
    BAIL_IF(
        !name || name->is_empty() || !kind ||
        *kind > U8(Render::Language::Binding::Kind::Resource) || !access ||
        *access > U8(Render::Language::Binding::Access::ReadWrite) ||
        (*kind == U8(Render::Language::Binding::Kind::Resource)) !=
            (*access != U8(Render::Language::Binding::Access::None)) ||
        !binding_contents.is_complete());
    Option<Library::Language::Field&> field;
    for (Abstract* declaration : program.get_declarations()) {
      auto candidate = declaration->select<Library::Language::Field>();
      if (candidate && candidate->get_name() == *name) {
        BAIL_IF(field);
        field = *candidate;
      }
    }
    BAIL_IF(!field);
    program.retain_shader_binding(
        *field, Render::Language::Binding::Kind(*kind),
        Render::Language::Binding::Access(*access));
  }

  auto uniform_count = contents.read_u32();
  BAIL_IF(!uniform_count);
  for (Count index = 0; index < *uniform_count; index++) {
    auto uniform_record = contents.read_record();
    BAIL_IF(!uniform_record || uniform_record->get_tag() != U16(Tag::Uniform));
    Reader uniform_contents(uniform_record->get_payload());
    auto name = uniform_contents.read_bytes();
    BAIL_IF(!name || name->is_empty() || !uniform_contents.is_complete());

    Option<Library::Language::Field&> field;
    for (Abstract* declaration : program.get_parameters().get_declarations()) {
      auto candidate = declaration->select<Library::Language::Field>();
      if (candidate && candidate->get_name() == *name) {
        BAIL_IF(field);
        field = *candidate;
      }
    }
    BAIL_IF(!field);
    program.retain_uniform(*field);
  }
  BAIL_IF(!contents.is_complete());
  return program;
}

auto Shader::Archive::Reader::read_bridge(
    Allocator::Arena& arena,
    Shader::Language::Monograph& monograph)
    -> Option<Shader::Language::Bridge&> {
  // A Bridge belongs to the Program that authored it, but its Type routes use
  // the complete Shader context so CPU and GPU endpoints can cross members.
  auto record = read_record();
  BAIL_IF(!record || record->get_tag() != U16(Tag::Bridge));
  Reader contents(record->get_payload());
  auto host_name = contents.read_bytes();
  BAIL_IF(!host_name || host_name->is_empty());
  Option<Shader::Language::Program&> host;
  for (Shader::Language::Program* program : monograph.get_programs()) {
    if (program->get_name() == *host_name) {
      BAIL_IF(host);
      host = *program;
    }
  }
  BAIL_IF(!host);
  auto definition = contents.read_definition(arena);
  auto cpu_payload = contents.read_bytes();
  auto gpu_payload = contents.read_bytes();
  auto direction = contents.read_u8();
  auto marshaling = contents.read_u8();
  auto synchronization = contents.read_u8();
  BAIL_IF(
      !definition || !cpu_payload || !gpu_payload || !direction ||
      *direction > U8(Shader::Language::Bridge::Direction::Bidirectional) ||
      !marshaling ||
      *marshaling > U8(Shader::Language::Bridge::Marshaling::Pack) ||
      !synchronization ||
      *synchronization > U8(Shader::Language::Bridge::Synchronization::Frame) ||
      !contents.is_complete());
  auto selected_direction = Shader::Language::Bridge::Direction(*direction);
  auto selected_marshaling = Shader::Language::Bridge::Marshaling(*marshaling);
  auto selected_synchronization =
      Shader::Language::Bridge::Synchronization(*synchronization);
  BAIL_IF(!bridge_attributes_match(
      definition->get_attributes(), selected_direction, selected_marshaling,
      selected_synchronization));
  auto cpu = Library::Archive::Reader::restore_type_reference(
      arena, *cpu_payload, monograph);
  auto gpu = Library::Archive::Reader::restore_type_reference(
      arena, *gpu_payload, monograph);
  BAIL_IF(!cpu || !gpu);

  auto& restored_definition = definition->create(arena, *host);
  auto& bridge = Shader::Language::Bridge::create(
      arena, restored_definition, *cpu, *gpu, selected_direction,
      selected_marshaling, selected_synchronization);
  BAIL_IF(!monograph.retain_bridge(bridge));
  return bridge;
}
