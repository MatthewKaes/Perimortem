// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/compiler/shader/program.hpp"

#include "perimortem/core/static/bytes.hpp"
#include "perimortem/core/writer/textual.hpp"

#include "tetrodotoxin/compiler/shader/metadata.hpp"
#include "tetrodotoxin/dialect/shader/metadata.hpp"
#include "tetrodotoxin/dialect/shader/quad.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Tetrodotoxin;
using namespace Tetrodotoxin::Compiler::Shader;

static constexpr Count quad_mesh_vertex_count =
    Dialect::Shader::QuadMesh::vertex_count;
using ShaderMetadata = Dialect::Shader::Metadata;

static auto count_bytes(Count value) -> Dynamic::Bytes {
  Dynamic::Bytes output;
  for (Count i = 0; i < 8; i++) {
    output.append(Bits_8((Bits_64(value) >> (i * 8)) & 0xFF));
  }
  return output;
}

static auto append_count(Dynamic::Bytes& output, Count value) -> void {
  Static::Bytes<32> count_buffer;
  Writer::Textual writer(count_buffer.get_access());
  writer << Bits_64(value);
  output.concat(View::Bytes(writer));
}

static auto symbol_name(View::Bytes prefix, View::Bytes suffix)
    -> Dynamic::Bytes {
  Dynamic::Bytes output;
  output.concat(prefix);
  output.concat(suffix);
  return output;
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
    Dynamic::Bytes& output,
    View::Bytes prefix,
    View::Bytes suffix) -> void {
  output.concat(prefix);
  output.concat(suffix);
}

static auto append_padding(Dynamic::Bytes& output, Count offset, Count size)
    -> void {
  if (size == 0) {
    return;
  }

  output.concat("  Bits_8 _ttx_padding_"_view);
  append_count(output, offset);
  output.append('[');
  append_count(output, size);
  output.concat("];\n"_view);
}

static auto append_cpp_field(
    Dynamic::Bytes& output,
    const ShaderMetadata::HostField& field) -> void {
  output.concat("  "_view);
  if (field.type_name == "Vec2D"_view) {
    output.concat("Real_32 "_view);
    output.concat(field.name);
    output.concat("[2];\n"_view);
    return;
  }

  if (field.type_name == "Size2D"_view) {
    output.concat("Real_32 "_view);
    output.concat(field.name);
    output.concat("[2];\n"_view);
    return;
  }

  if (field.type_name == "Vec3D"_view) {
    output.concat("Real_32 "_view);
    output.concat(field.name);
    output.concat("[3];\n"_view);
    return;
  }

  if (field.type_name == "Vec4D"_view || field.type_name == "Color"_view) {
    output.concat("Real_32 "_view);
    output.concat(field.name);
    output.concat("[4];\n"_view);
    return;
  }

  if (field.type_name == "Real_32"_view) {
    output.concat("Real_32 "_view);
    output.concat(field.name);
    output.concat(";\n"_view);
    return;
  }

  if (field.type_name == "Bits_32"_view) {
    output.concat("Bits_32 "_view);
    output.concat(field.name);
    output.concat(";\n"_view);
    return;
  }

  output.concat("Bits_8 "_view);
  output.concat(field.name);
  output.append('[');
  append_count(output, field.size);
  output.concat("];\n"_view);
}

static auto append_host_input_struct(
    Dynamic::Bytes& output,
    View::Bytes prefix,
    const ShaderMetadata::HostInputs& metadata) -> void {
  View::Vector<ShaderMetadata::HostField> fields = metadata.fields.get_view();
  if (fields.is_empty()) {
    return;
  }

  output.concat("struct "_view);
  output.concat(prefix);
  output.concat("_HostInputs {\n"_view);

  Count cursor = 0;
  for (Count i = 0; i < fields.get_size(); i++) {
    if (fields[i].offset > cursor) {
      append_padding(output, cursor, fields[i].offset - cursor);
    }

    append_cpp_field(output, fields[i]);
    cursor = fields[i].offset + fields[i].size;
  }

  if (metadata.byte_size > cursor) {
    append_padding(output, cursor, metadata.byte_size - cursor);
  }

  output.concat("};\n"_view);
  output.concat("static_assert(sizeof("_view);
  output.concat(prefix);
  output.concat("_HostInputs) == "_view);
  append_count(output, metadata.byte_size);
  output.concat(");\n\n"_view);
}

static auto append_render_type(Dynamic::Bytes& output, View::Bytes name)
    -> void {
  output.concat("Perimortem::Graphics::Render"_view);
  if (!name.is_empty()) {
    output.concat("::"_view);
    output.concat(name);
  }
}

static auto append_render_view(Dynamic::Bytes& output, View::Bytes name)
    -> void {
  output.concat("Perimortem::Core::View::Vector<"_view);
  append_render_type(output, name);
  output.concat(">()"_view);
}

static auto append_render_vector_type(
    Dynamic::Bytes& output,
    View::Bytes element_type,
    Count size) -> void {
  output.concat("Perimortem::Core::Static::Vector<"_view);
  append_render_type(output, element_type);
  output.concat(", "_view);
  append_count(output, size);
  output.append('>');
}

static auto append_render_stage(Dynamic::Bytes& output, View::Bytes name)
    -> void {
  append_render_type(output, "Stage"_view);
  output.concat("::"_view);
  output.concat(name);
}

static auto render_stage_name(ShaderMetadata::Stage stage) -> View::Bytes {
  switch (stage) {
  case ShaderMetadata::Stage::Vertex:
    return "Vertex"_view;
  case ShaderMetadata::Stage::Pixel:
    return "Pixel"_view;
  case ShaderMetadata::Stage::None:
    return View::Bytes();
  }

  return View::Bytes();
}

static auto append_render_program(
    Dynamic::Bytes& output,
    View::Bytes prefix,
    const ShaderMetadata::HostInputs& host_inputs,
    View::Vector<ShaderMetadata::Binding> descriptors) -> void {
  output.concat("namespace Internal {\n\n"_view);

  append_render_vector_type(output, "Module"_view, 2);
  output.append(' ');
  output.concat(prefix);
  output.concat("_modules(\n"_view);
  output.concat("  "_view);
  append_render_type(output, "Module"_view);
  output.concat("(\n      "_view);
  append_render_stage(output, "Vertex"_view);
  output.concat(", "_view);
  append_symbol(output, prefix, "_vert_spv"_view);
  output.concat(", &"_view);
  append_symbol(output, prefix, "_vert_spv_size"_view);
  output.concat(", \"main\"),\n  "_view);
  append_render_type(output, "Module"_view);
  output.concat("(\n      "_view);
  append_render_stage(output, "Pixel"_view);
  output.concat(", "_view);
  append_symbol(output, prefix, "_frag_spv"_view);
  output.concat(", &"_view);
  append_symbol(output, prefix, "_frag_spv_size"_view);
  output.concat(", \"main\")\n);\n\n"_view);

  View::Vector<ShaderMetadata::Stage> host_input_stages =
      host_inputs.stages.get_view();
  if (!host_input_stages.is_empty()) {
    append_render_vector_type(
        output, "Stage"_view, host_input_stages.get_size());
    output.append(' ');
    output.concat(prefix);
    output.concat("_host_input_stages(\n"_view);
    for (Count i = 0; i < host_input_stages.get_size(); i++) {
      output.concat("  "_view);
      append_render_stage(output, render_stage_name(host_input_stages[i]));
      if (i + 1 < host_input_stages.get_size()) {
        output.append(',');
      }
      output.append('\n');
    }
    output.concat(");\n\n"_view);

    append_render_vector_type(output, "HostInputRange"_view, 1);
    output.append(' ');
    output.concat(prefix);
    output.concat("_host_input_ranges(\n"_view);
    output.concat("  "_view);
    append_render_type(output, "HostInputRange"_view);
    output.concat("(0, sizeof("_view);
    output.concat(prefix);
    output.concat("_HostInputs), "_view);
    output.concat(prefix);
    output.concat("_host_input_stages)\n);\n\n"_view);
  }

  if (!descriptors.is_empty()) {
    append_render_vector_type(
        output, "DescriptorBinding"_view, descriptors.get_size());
    output.append(' ');
    output.concat(prefix);
    output.concat("_descriptor_bindings(\n"_view);
    for (Count i = 0; i < descriptors.get_size(); i++) {
      output.concat("  "_view);
      append_render_type(output, "DescriptorBinding"_view);
      output.concat("(\""_view);
      output.concat(descriptors[i].name);
      output.concat("\", "_view);
      append_count(output, descriptors[i].set);
      output.concat(", "_view);
      append_count(output, descriptors[i].slot);
      output.append(')');
      if (i + 1 < descriptors.get_size()) {
        output.append(',');
      }
      output.append('\n');
    }
    output.concat(");\n\n"_view);
  }

  View::Vector<ShaderMetadata::HostField> fields =
      host_inputs.fields.get_view();
  if (!fields.is_empty()) {
    append_render_vector_type(output, "HostField"_view, fields.get_size());
    output.append(' ');
    output.concat(prefix);
    output.concat("_host_fields(\n"_view);
    for (Count i = 0; i < fields.get_size(); i++) {
      output.concat("  "_view);
      append_render_type(output, "HostField"_view);
      output.concat("(\""_view);
      output.concat(fields[i].name);
      output.concat("\", "_view);
      append_count(output, fields[i].offset);
      output.concat(", "_view);
      append_count(output, fields[i].size);
      output.append(')');
      if (i + 1 < fields.get_size()) {
        output.append(',');
      }
      output.append('\n');
    }
    output.concat(");\n\n"_view);
  }

  output.concat("static constexpr "_view);
  append_render_type(output, "Program"_view);
  output.append(' ');
  output.concat(prefix);
  output.concat("_render_program(\n"_view);
  output.concat("  "_view);
  output.concat(prefix);
  output.concat("_modules,\n"_view);
  if (host_input_stages.is_empty()) {
    output.concat("  "_view);
    append_render_view(output, "HostInputRange"_view);
    output.concat(",\n"_view);
  } else {
    output.concat("  "_view);
    output.concat(prefix);
    output.concat("_host_input_ranges,\n"_view);
  }
  if (descriptors.is_empty()) {
    output.concat("  "_view);
    append_render_view(output, "DescriptorBinding"_view);
    output.concat(",\n"_view);
  } else {
    output.concat("  "_view);
    output.concat(prefix);
    output.concat("_descriptor_bindings,\n"_view);
  }
  if (fields.is_empty()) {
    output.concat("  "_view);
    append_render_view(output, "HostField"_view);
    output.concat(",\n"_view);
  } else {
    output.concat("  "_view);
    output.concat(prefix);
    output.concat("_host_fields,\n"_view);
  }
  output.concat("  "_view);
  append_count(output, quad_mesh_vertex_count);
  output.concat("\n);\n\n"_view);

  output.concat("}  // namespace Internal\n\n"_view);

  output.concat("static constexpr Perimortem::Graphics::Render "_view);
  output.concat(prefix);
  output.concat("_renderer(Internal::"_view);
  output.concat(prefix);
  output.concat("_render_program);\n\n"_view);
}

auto Tetrodotoxin::Compiler::Shader::add_program_symbols(
    Tetrodotoxin::Compiler::Compiler& compiler,
    Resolution::Resolver& resolver,
    const Resolution::Record& record,
    const CompiledProgram& program) -> void {
  ShaderMetadata shader;

  Dynamic::Bytes host_inputs;
  Dynamic::Bytes descriptors;
  write_host_input_metadata(shader.host_inputs(resolver, record), host_inputs);
  write_descriptor_metadata(shader.descriptors(resolver, record), descriptors);

  Dynamic::Bytes vertex_size = count_bytes(program.vertex_spirv.get_size());
  Dynamic::Bytes pixel_size = count_bytes(program.pixel_spirv.get_size());
  Dynamic::Bytes host_inputs_size = count_bytes(host_inputs.get_size());
  Dynamic::Bytes descriptors_size = count_bytes(descriptors.get_size());

  add_read_only(
      compiler, program.symbol_prefix, "_vert_spv"_view, program.vertex_spirv,
      4);
  add_read_only(
      compiler, program.symbol_prefix, "_vert_spv_size"_view, vertex_size, 8);
  add_read_only(
      compiler, program.symbol_prefix, "_frag_spv"_view, program.pixel_spirv,
      4);
  add_read_only(
      compiler, program.symbol_prefix, "_frag_spv_size"_view, pixel_size, 8);
  add_read_only(
      compiler, program.symbol_prefix, "_host_inputs"_view, host_inputs,
      8);
  add_read_only(
      compiler, program.symbol_prefix, "_host_inputs_size"_view,
      host_inputs_size, 8);
  add_read_only(
      compiler, program.symbol_prefix, "_descriptors"_view, descriptors, 8);
  add_read_only(
      compiler, program.symbol_prefix, "_descriptors_size"_view,
      descriptors_size, 8);
}

auto Tetrodotoxin::Compiler::Shader::write_program_header(
    Resolution::Resolver& resolver,
    const Resolution::Record& record,
    View::Bytes symbol_prefix,
    Dynamic::Bytes& output) -> void {
  ShaderMetadata shader;

  output.concat(
      "//    This file is generated by the Tetrodotoxin compiler\n"_view);
  output.concat("//         Manual edits can break the C++/TTX ABI\n\n"_view);
  output.concat("#pragma once\n\n"_view);
  output.concat("#include \"perimortem/core/static/vector.hpp\"\n"_view);
  output.concat("#include \"perimortem/graphics/render.hpp\"\n\n"_view);

  output.concat("namespace Ttx {\n\n"_view);
  output.concat("extern \"C\" {\n\n"_view);

  output.concat("extern const Bits_32 "_view);
  append_symbol(output, symbol_prefix, "_vert_spv"_view);
  output.concat("[];\n"_view);
  output.concat("extern const Count "_view);
  append_symbol(output, symbol_prefix, "_vert_spv_size"_view);
  output.concat(";\n"_view);
  output.concat("extern const Bits_32 "_view);
  append_symbol(output, symbol_prefix, "_frag_spv"_view);
  output.concat("[];\n"_view);
  output.concat("extern const Count "_view);
  append_symbol(output, symbol_prefix, "_frag_spv_size"_view);
  output.concat(";\n"_view);
  output.concat("extern const Bits_8 "_view);
  append_symbol(output, symbol_prefix, "_host_inputs"_view);
  output.concat("[];\n"_view);
  output.concat("extern const Count "_view);
  append_symbol(output, symbol_prefix, "_host_inputs_size"_view);
  output.concat(";\n"_view);
  output.concat("extern const Bits_8 "_view);
  append_symbol(output, symbol_prefix, "_descriptors"_view);
  output.concat("[];\n"_view);
  output.concat("extern const Count "_view);
  append_symbol(output, symbol_prefix, "_descriptors_size"_view);
  output.concat(";\n\n"_view);

  output.concat("}  // extern \"C\"\n\n"_view);

  const ShaderMetadata::HostInputs host_inputs =
      shader.host_inputs(resolver, record);
  append_host_input_struct(output, symbol_prefix, host_inputs);

  Dynamic::Vector<ShaderMetadata::Binding> descriptors =
      shader.descriptors(resolver, record);
  for (Count i = 0; i < descriptors.get_size(); i++) {
    output.concat("static constexpr Count "_view);
    output.concat(symbol_prefix);
    output.append('_');
    output.concat(descriptors[i].name);
    output.concat("_set = "_view);
    append_count(output, descriptors[i].set);
    output.concat(";\n"_view);
    output.concat("static constexpr Count "_view);
    output.concat(symbol_prefix);
    output.append('_');
    output.concat(descriptors[i].name);
    output.concat("_slot = "_view);
    append_count(output, descriptors[i].slot);
    output.concat(";\n"_view);
  }

  if (descriptors.get_size() > 0) {
    output.append('\n');
  }
  append_render_program(output, symbol_prefix, host_inputs, descriptors);

  output.concat("\n}  // namespace Ttx\n"_view);
}
