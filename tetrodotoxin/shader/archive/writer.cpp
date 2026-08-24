// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/shader/archive/writer.hpp"

#include "perimortem/core/writer/binary.hpp"

#include "perimortem/serialization/stream/binary.hpp"

#include "tetrodotoxin/library/archive/writer.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Perimortem::Serialization;
using namespace Ttx::Concept;
using namespace Tetrodotoxin;

using Appender = Stream::Binary<Data::ByteOrder::Little, Dynamic::Bytes>;
using Patcher = Perimortem::Core::Writer::Binary<Data::ByteOrder::Little>;

enum class ShaderWriterAttributeValue : U8 {
  Empty,
  Bytes,
  Unsigned,
  Signed,
  Real,
  Flag,
};

Shader::Archive::Writer::Writer(
    Tetrodotoxin::Language::Persistence::Profile profile)
    : profile(profile) {
  Appender appender(bytes);
  appender << "TTXS"_view;
  appender << U16(1);
  appender << U8(profile);
  appender << U8(0);
}

auto Shader::Archive::Writer::encode(
    const Shader::Language::Monograph& monograph,
    Tetrodotoxin::Language::Persistence::Profile profile)
    -> Option<Dynamic::Bytes> {
  BAIL_IF(!monograph.is_finalized());

  Writer writer(profile);
  auto record = writer.begin(Tag::Monograph);
  BAIL_IF(!writer.write(monograph.get_documentation()));
  for (const Reference<Shader::Language::Program>& retained :
       monograph.get_programs()) {
    const Shader::Language::Program& program = retained.get();
    if (!writer.public_only() || program.get_definition().is_published()) {
      BAIL_IF(!writer.write(program));
    }
  }
  for (const Reference<Shader::Language::Bridge>& retained :
       monograph.get_bridges()) {
    const Shader::Language::Bridge& bridge = retained.get();
    if (!writer.public_only() || bridge.get_definition().is_published()) {
      BAIL_IF(!writer.write(bridge));
    }
  }
  BAIL_IF(!writer.finish(record));
  return Data::take(writer.bytes);
}

auto Shader::Archive::Writer::begin(Tag tag) -> Record {
  Count offset = bytes.get_size();
  Appender appender(bytes);
  appender << U16(tag);
  appender << U16(0);
  appender << U32(0);
  return Record(offset);
}

auto Shader::Archive::Writer::finish(Record record) -> Bool {
  Count offset = record.get_offset();
  BAIL_IF(offset > bytes.get_size() || bytes.get_size() - offset < 8);
  Count size = bytes.get_size() - offset - 8;
  BAIL_IF(size > U32(-1));
  Patcher patcher(bytes.get_access().slice(offset + 4, 4));
  patcher << U32(size);
  return patcher.is_valid();
}

auto Shader::Archive::Writer::write(U8 value) -> void {
  Appender(bytes) << value;
}

auto Shader::Archive::Writer::write(U32 value) -> void {
  Appender(bytes) << value;
}

auto Shader::Archive::Writer::write(U64 value) -> void {
  Appender(bytes) << value;
}

auto Shader::Archive::Writer::write(S64 value) -> void {
  Appender(bytes) << value;
}

auto Shader::Archive::Writer::write(R64 value) -> void {
  Appender(bytes) << value;
}

auto Shader::Archive::Writer::write(View::Bytes value) -> Bool {
  BAIL_IF(value.get_size() > U32(-1));
  Appender appender(bytes);
  appender << U32(value.get_size());
  appender << value;
  return True;
}

auto Shader::Archive::Writer::write(const Documentation& value) -> Bool {
  BAIL_IF(value.line_count() > U32(-1));
  write(U32(value.line_count()));
  for (Count index = 0; index < value.line_count(); index++) {
    BAIL_IF(!write(value.get_line(index)));
  }
  return True;
}

auto Shader::Archive::Writer::write(
    View::Vector<Tetrodotoxin::Language::Attribute> attributes) -> Bool {
  BAIL_IF(attributes.get_size() > U32(-1));
  write(U32(attributes.get_size()));
  for (const Tetrodotoxin::Language::Attribute& attribute : attributes) {
    BAIL_IF(!write(attribute.get_key()));
    Bool written = attribute.get_value().visit(
        [&]() -> Bool {
          write(U8(ShaderWriterAttributeValue::Empty));
          return True;
        },
        [&](View::Bytes selected) -> Bool {
          write(U8(ShaderWriterAttributeValue::Bytes));
          return write(selected);
        },
        [&](U64 selected) -> Bool {
          write(U8(ShaderWriterAttributeValue::Unsigned));
          write(selected);
          return True;
        },
        [&](S64 selected) -> Bool {
          write(U8(ShaderWriterAttributeValue::Signed));
          write(selected);
          return True;
        },
        [&](R64 selected) -> Bool {
          write(U8(ShaderWriterAttributeValue::Real));
          write(selected);
          return True;
        },
        [&](Bool selected) -> Bool {
          write(U8(ShaderWriterAttributeValue::Flag));
          write(U8(selected ? 1 : 0));
          return True;
        });
    BAIL_IF(!written);
  }
  return True;
}

auto Shader::Archive::Writer::write(
    const Tetrodotoxin::Language::Definition& definition) -> Bool {
  BAIL_IF(
      !write(definition.get_documentation()) ||
      !write(definition.get_attributes()) || !write(definition.get_name()));
  write(U8(definition.get_visibility()));
  return True;
}

auto Shader::Archive::Writer::write(const Shader::Language::Program& program)
    -> Bool {
  // Library writes the executable declaration surface through its own schema.
  // Shader wraps those bytes with only the relationship facts it owns.
  auto declarations =
      Library::Archive::Writer::encode_declarations(program, profile);
  BAIL_IF(!declarations || program.get_bindings().get_size() > U32(-1));

  auto record = begin(Tag::Program);
  BAIL_IF(
      !write(program.get_definition()) ||
      !write(program.get_contract_reference().get_route()) ||
      !write(declarations->get_view()));
  Count included = 0;
  for (const Shader::Language::Binding& binding : program.get_bindings()) {
    if (!public_only() || binding.get_definition().is_published()) {
      included++;
    }
  }
  BAIL_IF(included > U32(-1));
  write(U32(included));
  for (const Shader::Language::Binding& binding : program.get_bindings()) {
    if (public_only() && !binding.get_definition().is_published()) {
      continue;
    }
    auto binding_record = begin(Tag::Binding);
    BAIL_IF(!write(binding.get_field().get_name()));
    write(U8(binding.get_kind()));
    BAIL_IF(!finish(binding_record));
  }
  return finish(record);
}

auto Shader::Archive::Writer::write(const Shader::Language::Bridge& bridge)
    -> Bool {
  // Endpoint routes remain Library payloads because Generic arguments and
  // literal facts belong to that Type system. Shader records only their policy.
  auto cpu = Library::Archive::Writer::encode_type_reference(
      bridge.get_cpu_reference(), profile);
  auto gpu = Library::Archive::Writer::encode_type_reference(
      bridge.get_gpu_reference(), profile);
  BAIL_IF(!cpu || !gpu);

  auto record = begin(Tag::Bridge);
  BAIL_IF(
      !write(bridge.get_definition().get_host().get_name()) ||
      !write(bridge.get_definition()) || !write(cpu->get_view()) ||
      !write(gpu->get_view()));
  write(U8(bridge.get_direction()));
  write(U8(bridge.get_marshaling()));
  write(U8(bridge.get_synchronization()));
  return finish(record);
}
