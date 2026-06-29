// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/compiler/shader/terminal.hpp"

#include "perimortem/core/static/bytes.hpp"
#include "perimortem/core/writer/textual.hpp"

#include "tetrodotoxin/compiler/shader/metadata.hpp"
#include "tetrodotoxin/dialect/shader/facts.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Tetrodotoxin;
using namespace Tetrodotoxin::Compiler::Shader;

static constexpr Count v1_vertex_count = 6;

static auto count_bytes(Count value) -> Dynamic::Bytes {
  Dynamic::Bytes out;
  for (Count i = 0; i < 8; i++) {
    out.append(Bits_8((Bits_64(value) >> (i * 8)) & 0xFF));
  }
  return out;
}

static auto append_count(Dynamic::Bytes& out, Count value) -> void {
  Static::Bytes<32> count_buffer;
  Writer::Textual writer(count_buffer.get_access());
  writer << Bits_64(value);
  out.concat(View::Bytes(writer));
}

static auto symbol_name(View::Bytes prefix, View::Bytes suffix)
    -> Dynamic::Bytes {
  Dynamic::Bytes out;
  out.concat(prefix);
  out.concat(suffix);
  return out;
}

static auto add_read_only(
    Tetrodotoxin::Compiler::Compiler& compiler,
    View::Bytes prefix,
    View::Bytes suffix,
    View::Bytes data,
    Count alignment) -> void {
  Dynamic::Bytes name = symbol_name(prefix, suffix);
  compiler.add_read_only_data(name, data, alignment);
}

static auto append_symbol(
    Dynamic::Bytes& out,
    View::Bytes prefix,
    View::Bytes suffix) -> void {
  out.concat(prefix);
  out.concat(suffix);
}

static auto append_padding(Dynamic::Bytes& out, Count offset, Count size)
    -> void {
  if (size == 0) {
    return;
  }

  out.concat("  Bits_8 _ttx_padding_"_view);
  append_count(out, offset);
  out.append('[');
  append_count(out, size);
  out.concat("];\n"_view);
}

static auto append_cpp_field(
    Dynamic::Bytes& out,
    const Dialect::Shader::HostField& field) -> void {
  out.concat("  "_view);
  if (field.type_name == "Vec2D"_view) {
    out.concat("Real_32 "_view);
    out.concat(field.name);
    out.concat("[2];\n"_view);
    return;
  }

  if (field.type_name == "Vec3D"_view) {
    out.concat("Real_32 "_view);
    out.concat(field.name);
    out.concat("[3];\n"_view);
    return;
  }

  if (field.type_name == "Vec4D"_view || field.type_name == "Color"_view) {
    out.concat("Real_32 "_view);
    out.concat(field.name);
    out.concat("[4];\n"_view);
    return;
  }

  if (field.type_name == "Real_32"_view) {
    out.concat("Real_32 "_view);
    out.concat(field.name);
    out.concat(";\n"_view);
    return;
  }

  if (field.type_name == "Bits_32"_view) {
    out.concat("Bits_32 "_view);
    out.concat(field.name);
    out.concat(";\n"_view);
    return;
  }

  out.concat("Bits_8 "_view);
  out.concat(field.name);
  out.append('[');
  append_count(out, field.size);
  out.concat("];\n"_view);
}

static auto shader_object_property(View::Bytes name) -> View::Bytes {
  if (name == "position"_view) {
    return "ShaderObjectProperty::Position"_view;
  }
  if (name == "half_size"_view) {
    return "ShaderObjectProperty::HalfSize"_view;
  }
  if (name == "alpha"_view) {
    return "ShaderObjectProperty::Alpha"_view;
  }

  return View::Bytes();
}

static auto count_shader_object_properties(
    const Dialect::Shader::PushConstantMetadata& metadata) -> Count {
  Count result = 0;
  View::Vector<Dialect::Shader::HostField> fields = metadata.fields.get_view();
  for (Count i = 0; i < fields.get_size(); i++) {
    if (!shader_object_property(fields[i].name).is_empty()) {
      result++;
    }
  }

  return result;
}

static auto append_push_struct(
    Dynamic::Bytes& out,
    View::Bytes prefix,
    const Dialect::Shader::PushConstantMetadata& metadata) -> void {
  View::Vector<Dialect::Shader::PushConstantBlock> blocks =
      metadata.blocks.get_view();
  View::Vector<Dialect::Shader::HostField> fields = metadata.fields.get_view();
  for (Count i = 0; i < blocks.get_size(); i++) {
    out.concat("struct "_view);
    out.concat(prefix);
    out.append('_');
    out.concat(blocks[i].name);
    out.concat(" {\n"_view);

    Count cursor = 0;
    Count field_end = blocks[i].field_start + blocks[i].field_count;
    for (Count j = blocks[i].field_start;
         j < field_end && j < fields.get_size(); j++) {
      if (fields[j].offset > cursor) {
        append_padding(out, cursor, fields[j].offset - cursor);
      }

      append_cpp_field(out, fields[j]);
      cursor = fields[j].offset + fields[j].size;
    }

    if (blocks[i].byte_size > cursor) {
      append_padding(out, cursor, blocks[i].byte_size - cursor);
    }

    out.concat("};\n"_view);
    out.concat("static_assert(sizeof("_view);
    out.concat(prefix);
    out.append('_');
    out.concat(blocks[i].name);
    out.concat(") == "_view);
    append_count(out, blocks[i].byte_size);
    out.concat(");\n\n"_view);
  }
}

static auto append_shader_program(
    Dynamic::Bytes& out,
    View::Bytes prefix,
    const Dialect::Shader::PushConstantMetadata& push_constants,
    View::Vector<Dialect::Shader::DescriptorBinding> descriptors) -> void {
  out.concat("namespace Internal {\n\n"_view);

  out.concat("static constexpr ShaderModule "_view);
  out.concat(prefix);
  out.concat("_modules[] = {\n"_view);
  out.concat("  {ShaderStage::Vertex, "_view);
  append_symbol(out, prefix, "_vert_spv"_view);
  out.concat(", &"_view);
  append_symbol(out, prefix, "_vert_spv_size"_view);
  out.concat(", \"main\"},\n"_view);
  out.concat("  {ShaderStage::Pixel, "_view);
  append_symbol(out, prefix, "_frag_spv"_view);
  out.concat(", &"_view);
  append_symbol(out, prefix, "_frag_spv_size"_view);
  out.concat(", \"main\"},\n"_view);
  out.concat("};\n\n"_view);

  View::Vector<Dialect::Shader::PushConstantBlock> blocks =
      push_constants.blocks.get_view();
  if (!blocks.is_empty()) {
    out.concat("static constexpr ShaderPushConstantRange "_view);
    out.concat(prefix);
    out.concat("_push_constant_ranges[] = {\n"_view);
    for (Count i = 0; i < blocks.get_size(); i++) {
      out.concat("  {"_view);
      append_count(out, 0);
      out.concat(", sizeof("_view);
      out.concat(prefix);
      out.append('_');
      out.concat(blocks[i].name);
      out.concat("), shader_stage_vertex | shader_stage_pixel},\n"_view);
    }
    out.concat("};\n\n"_view);
  }

  if (!descriptors.is_empty()) {
    out.concat("static constexpr ShaderDescriptorBinding "_view);
    out.concat(prefix);
    out.concat("_descriptor_bindings[] = {\n"_view);
    for (Count i = 0; i < descriptors.get_size(); i++) {
      out.concat("  {\""_view);
      out.concat(descriptors[i].name);
      out.concat("\", "_view);
      append_count(out, descriptors[i].set);
      out.concat(", "_view);
      append_count(out, descriptors[i].slot);
      out.concat("},\n"_view);
    }
    out.concat("};\n\n"_view);
  }

  const Count object_property_count =
      count_shader_object_properties(push_constants);
  if (object_property_count > 0) {
    View::Vector<Dialect::Shader::HostField> fields =
        push_constants.fields.get_view();
    out.concat("static constexpr ShaderObjectPropertyBinding "_view);
    out.concat(prefix);
    out.concat("_object_properties[] = {\n"_view);
    for (Count i = 0; i < fields.get_size(); i++) {
      View::Bytes property = shader_object_property(fields[i].name);
      if (property.is_empty()) {
        continue;
      }

      out.concat("  {"_view);
      out.concat(property);
      out.concat(", "_view);
      append_count(out, fields[i].offset);
      out.concat(", "_view);
      append_count(out, fields[i].size);
      out.concat("},\n"_view);
    }
    out.concat("};\n\n"_view);
  }

  out.concat("static constexpr ShaderProgramInfo "_view);
  out.concat(prefix);
  out.concat("_shader_info = {\n"_view);
  out.concat("  "_view);
  out.concat(prefix);
  out.concat("_modules,\n"_view);
  out.concat("  2,\n"_view);
  if (blocks.is_empty()) {
    out.concat("  nullptr,\n  0,\n"_view);
  } else {
    out.concat("  "_view);
    out.concat(prefix);
    out.concat("_push_constant_ranges,\n  "_view);
    append_count(out, blocks.get_size());
    out.concat(",\n"_view);
  }
  if (descriptors.is_empty()) {
    out.concat("  nullptr,\n  0,\n"_view);
  } else {
    out.concat("  "_view);
    out.concat(prefix);
    out.concat("_descriptor_bindings,\n  "_view);
    append_count(out, descriptors.get_size());
    out.concat(",\n"_view);
  }
  if (object_property_count == 0) {
    out.concat("  nullptr,\n  0,\n"_view);
  } else {
    out.concat("  "_view);
    out.concat(prefix);
    out.concat("_object_properties,\n  "_view);
    append_count(out, object_property_count);
    out.concat(",\n"_view);
  }
  out.concat("  "_view);
  append_count(out, v1_vertex_count);
  out.concat(",\n};\n\n"_view);

  out.concat("}  // namespace Internal\n\n"_view);

  out.concat("static constexpr Shader "_view);
  out.concat(prefix);
  out.concat("_shader = {&Internal::"_view);
  out.concat(prefix);
  out.concat("_shader_info};\n\n"_view);
}

auto Tetrodotoxin::Compiler::Shader::add_v1_terminal_symbols(
    Tetrodotoxin::Compiler::Compiler& compiler,
    const Resolution::Record& record,
    const V1Terminal& terminal) -> void {
  Dialect::Shader::Facts shader;

  Dynamic::Bytes push_constants;
  Dynamic::Bytes descriptors;
  write_push_constant_metadata(shader.push_constants(record), push_constants);
  write_descriptor_metadata(shader.descriptors(record), descriptors);

  Dynamic::Bytes vertex_size = count_bytes(terminal.vertex_spirv.get_size());
  Dynamic::Bytes pixel_size = count_bytes(terminal.pixel_spirv.get_size());
  Dynamic::Bytes push_constants_size = count_bytes(push_constants.get_size());
  Dynamic::Bytes descriptors_size = count_bytes(descriptors.get_size());

  add_read_only(
      compiler, terminal.symbol_prefix, "_vert_spv"_view, terminal.vertex_spirv,
      4);
  add_read_only(
      compiler, terminal.symbol_prefix, "_vert_spv_size"_view, vertex_size, 8);
  add_read_only(
      compiler, terminal.symbol_prefix, "_frag_spv"_view, terminal.pixel_spirv,
      4);
  add_read_only(
      compiler, terminal.symbol_prefix, "_frag_spv_size"_view, pixel_size, 8);
  add_read_only(
      compiler, terminal.symbol_prefix, "_push_constants"_view, push_constants,
      8);
  add_read_only(
      compiler, terminal.symbol_prefix, "_push_constants_size"_view,
      push_constants_size, 8);
  add_read_only(
      compiler, terminal.symbol_prefix, "_descriptors"_view, descriptors, 8);
  add_read_only(
      compiler, terminal.symbol_prefix, "_descriptors_size"_view,
      descriptors_size, 8);
}

auto Tetrodotoxin::Compiler::Shader::write_v1_terminal_header(
    const Resolution::Record& record,
    View::Bytes symbol_prefix,
    Dynamic::Bytes& out) -> void {
  Dialect::Shader::Facts shader;

  out.concat(
      "//    This file is generated by the Tetrodotoxin compiler\n"_view);
  out.concat("//         Manual edits can break the C++/TTX ABI\n\n"_view);
  out.concat("#pragma once\n\n"_view);
  out.concat("#include \"perimortem/graphics/ttx_shader.hpp\"\n\n"_view);

  out.concat("namespace Ttx {\n\n"_view);
  out.concat("extern \"C\" {\n\n"_view);

  out.concat("extern const Bits_32 "_view);
  append_symbol(out, symbol_prefix, "_vert_spv"_view);
  out.concat("[];\n"_view);
  out.concat("extern const Count "_view);
  append_symbol(out, symbol_prefix, "_vert_spv_size"_view);
  out.concat(";\n"_view);
  out.concat("extern const Bits_32 "_view);
  append_symbol(out, symbol_prefix, "_frag_spv"_view);
  out.concat("[];\n"_view);
  out.concat("extern const Count "_view);
  append_symbol(out, symbol_prefix, "_frag_spv_size"_view);
  out.concat(";\n"_view);
  out.concat("extern const Bits_8 "_view);
  append_symbol(out, symbol_prefix, "_push_constants"_view);
  out.concat("[];\n"_view);
  out.concat("extern const Count "_view);
  append_symbol(out, symbol_prefix, "_push_constants_size"_view);
  out.concat(";\n"_view);
  out.concat("extern const Bits_8 "_view);
  append_symbol(out, symbol_prefix, "_descriptors"_view);
  out.concat("[];\n"_view);
  out.concat("extern const Count "_view);
  append_symbol(out, symbol_prefix, "_descriptors_size"_view);
  out.concat(";\n\n"_view);

  out.concat("}  // extern \"C\"\n\n"_view);

  const Dialect::Shader::PushConstantMetadata push_constants =
      shader.push_constants(record);
  append_push_struct(out, symbol_prefix, push_constants);

  Dynamic::Vector<Dialect::Shader::DescriptorBinding> descriptors =
      shader.descriptors(record);
  for (Count i = 0; i < descriptors.get_size(); i++) {
    out.concat("static constexpr Count "_view);
    out.concat(symbol_prefix);
    out.append('_');
    out.concat(descriptors[i].name);
    out.concat("_set = "_view);
    append_count(out, descriptors[i].set);
    out.concat(";\n"_view);
    out.concat("static constexpr Count "_view);
    out.concat(symbol_prefix);
    out.append('_');
    out.concat(descriptors[i].name);
    out.concat("_slot = "_view);
    append_count(out, descriptors[i].slot);
    out.concat(";\n"_view);
  }

  if (descriptors.get_size() > 0) {
    out.append('\n');
  }
  append_shader_program(out, symbol_prefix, push_constants, descriptors);

  out.concat("\n}  // namespace Ttx\n"_view);
}
