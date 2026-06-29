// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/compiler/shader/metadata.hpp"

#include "perimortem/core/static/bytes.hpp"
#include "perimortem/core/writer/textual.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Tetrodotoxin;
using namespace Tetrodotoxin::Compiler::Shader;

static auto append_count(Dynamic::Bytes& out, Count value) -> void {
  Static::Bytes<32> count_buffer;
  Writer::Textual writer(count_buffer.get_access());
  writer << Bits_64(value);
  out.concat(View::Bytes(writer));
}

static auto append_field(
    Dynamic::Bytes& out,
    View::Bytes label,
    View::Bytes value) -> void {
  out.concat(label);
  out.append('=');
  out.concat(value);
}

static auto append_field(Dynamic::Bytes& out, View::Bytes label, Count value)
    -> void {
  out.concat(label);
  out.append('=');
  append_count(out, value);
}

auto Tetrodotoxin::Compiler::Shader::write_push_constant_metadata(
    const Dialect::Shader::PushConstantMetadata& metadata,
    Dynamic::Bytes& out) -> void {
  out.concat("TTX_SHADER_PUSH_CONSTANTS_V1\n"_view);

  auto blocks = metadata.blocks.get_view();
  auto fields = metadata.fields.get_view();
  for (Count i = 0; i < blocks.get_size(); i++) {
    out.concat("block "_view);
    append_field(out, "name"_view, blocks[i].name);
    out.append(' ');
    append_field(out, "size"_view, blocks[i].byte_size);
    out.append(' ');
    append_field(out, "alignment"_view, blocks[i].alignment);
    out.append(' ');
    append_field(out, "fields"_view, blocks[i].field_count);
    out.append('\n');

    Count field_end = blocks[i].field_start + blocks[i].field_count;
    for (Count j = blocks[i].field_start;
         j < field_end && j < fields.get_size(); j++) {
      out.concat("field "_view);
      append_field(out, "name"_view, fields[j].name);
      out.append(' ');
      append_field(out, "type"_view, fields[j].type_name);
      out.append(' ');
      append_field(out, "offset"_view, fields[j].offset);
      out.append(' ');
      append_field(out, "size"_view, fields[j].size);
      out.append(' ');
      append_field(out, "alignment"_view, fields[j].alignment);
      out.append('\n');
    }
  }
}

auto Tetrodotoxin::Compiler::Shader::write_descriptor_metadata(
    View::Vector<Dialect::Shader::DescriptorBinding> metadata,
    Dynamic::Bytes& out) -> void {
  out.concat("TTX_SHADER_DESCRIPTORS_V1\n"_view);

  for (Count i = 0; i < metadata.get_size(); i++) {
    out.concat("descriptor "_view);
    append_field(out, "name"_view, metadata[i].name);
    out.append(' ');
    append_field(out, "type"_view, metadata[i].type_name);
    out.append(' ');
    append_field(out, "set"_view, metadata[i].set);
    out.append(' ');
    append_field(out, "slot"_view, metadata[i].slot);
    out.append('\n');
  }
}
