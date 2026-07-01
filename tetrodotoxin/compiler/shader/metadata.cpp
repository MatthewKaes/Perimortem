// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/compiler/shader/metadata.hpp"

#include "perimortem/core/static/bytes.hpp"
#include "perimortem/core/writer/textual.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Tetrodotoxin;
using namespace Tetrodotoxin::Compiler::Shader;

static auto append_count(Dynamic::Bytes& output, Count value) -> void {
  Static::Bytes<32> count_buffer;
  Writer::Textual writer(count_buffer.get_access());
  writer << Bits_64(value);
  output.concat(View::Bytes(writer));
}

static auto append_field(
    Dynamic::Bytes& output,
    View::Bytes label,
    View::Bytes value) -> void {
  output.concat(label);
  output.append('=');
  output.concat(value);
}

static auto append_field(Dynamic::Bytes& output, View::Bytes label, Count value)
    -> void {
  output.concat(label);
  output.append('=');
  append_count(output, value);
}

auto Tetrodotoxin::Compiler::Shader::write_host_input_metadata(
    const Dialect::Shader::Metadata::HostInputs& metadata,
    Dynamic::Bytes& output) -> void {
  output.concat("TTX_SHADER_HOST_INPUTS\n"_view);
  append_field(output, "size"_view, metadata.get_byte_size());
  output.append(' ');
  append_field(output, "alignment"_view, metadata.get_alignment());
  output.append(' ');
  append_field(output, "fields"_view, metadata.get_fields().get_size());
  output.append('\n');

  View::Vector<Dialect::Shader::Metadata::HostInputs::Field> fields =
      metadata.get_fields();
  for (Count i = 0; i < fields.get_size(); i++) {
    output.concat("field "_view);
    append_field(output, "name"_view, fields[i].get_name());
    output.append(' ');
    append_field(output, "type"_view, fields[i].get_type_name());
    output.append(' ');
    append_field(output, "offset"_view, fields[i].get_offset());
    output.append(' ');
    append_field(output, "size"_view, fields[i].get_size());
    output.append(' ');
    append_field(output, "alignment"_view, fields[i].get_alignment());
    output.append('\n');
  }
}

auto Tetrodotoxin::Compiler::Shader::write_descriptor_metadata(
    View::Vector<Dialect::Shader::Metadata::Binding> metadata,
    Dynamic::Bytes& output) -> void {
  output.concat("TTX_SHADER_DESCRIPTORS\n"_view);

  for (Count i = 0; i < metadata.get_size(); i++) {
    output.concat("descriptor "_view);
    append_field(output, "name"_view, metadata[i].get_name());
    output.append(' ');
    append_field(output, "type"_view, metadata[i].get_type_name());
    output.append(' ');
    append_field(output, "set"_view, metadata[i].get_set());
    output.append(' ');
    append_field(output, "slot"_view, metadata[i].get_slot());
    output.append('\n');
  }
}
