// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/compiler/shader/spirv.hpp"

#include "perimortem/core/static/vector.hpp"
#include "perimortem/core/data.hpp"

#include "tetrodotoxin/compiler/assembler/spirv.hpp"
#include "tetrodotoxin/dialect/shader/quad.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Tetrodotoxin;
using namespace Tetrodotoxin::Compiler::Assembler;
using namespace Tetrodotoxin::Compiler::Shader;

using QuadMesh = Dialect::Shader::QuadMesh;
using Vec2Literal = Dialect::Shader::Vec2Literal;

static constexpr Bits_32 id_bound = 256;
static constexpr Count quad_vertex_count = QuadMesh::vertex_count;

class IdPool {
 public:
  auto take() -> Bits_32 { return next++; }

  Bits_32 next = 1;
};

class PushInfo {
 public:
  Dialect::Shader::PushConstantMetadata metadata;
  Count block_index = 0;
  Count position_index = 0;
  Count half_size_index = 1;
  Count alpha_index = 2;
  Bool valid = False;
};

static auto real32_bits(Real_32 value) -> Bits_32 {
  Bits_32 out = 0;
  Data::copy(Data::cast<Bits_8>(&out), &value, 1);
  return out;
}

static auto find_member_with_builtin(
    const Syntax::Type::Declaration& type,
    const Dialect::Shader::Facts& shader,
    View::Bytes builtin_name) -> const Syntax::Ast::Member* {
  View::Vector<Syntax::Ast::Member*> members = type.get_scope();
  for (Count i = 0; i < members.get_size(); i++) {
    if (!members[i]) {
      continue;
    }

    Dialect::Shader::Builtin builtin = shader.builtin_of(*members[i]);
    if (builtin.present && builtin.name == builtin_name) {
      return members[i];
    }
  }

  return nullptr;
}

static auto find_interface_field(
    View::Vector<Dialect::Shader::InterfaceField> fields,
    View::Bytes name) -> const Dialect::Shader::InterfaceField* {
  for (Count i = 0; i < fields.get_size(); i++) {
    if (fields[i].name == name) {
      return &fields[i];
    }
  }

  return nullptr;
}

static auto first_user_output(
    View::Vector<Dialect::Shader::InterfaceField> fields)
    -> const Dialect::Shader::InterfaceField* {
  for (Count i = 0; i < fields.get_size(); i++) {
    if (!fields[i].builtin.present) {
      return &fields[i];
    }
  }

  return nullptr;
}

static auto find_field_index(
    View::Vector<Dialect::Shader::HostField> fields,
    View::Bytes name,
    Count fallback) -> Count {
  for (Count i = 0; i < fields.get_size(); i++) {
    if (fields[i].name == name) {
      return i;
    }
  }

  return fallback;
}

static auto load_push_info(const Resolution::Record& record) -> PushInfo {
  Dialect::Shader::Facts shader;
  PushInfo info;
  info.metadata = shader.push_constants(record);
  if (info.metadata.blocks.get_size() == 0) {
    return info;
  }

  const auto& block = info.metadata.blocks.get_view()[info.block_index];
  View::Vector<Dialect::Shader::HostField> fields =
      info.metadata.fields.get_view().slice(
          block.field_start, block.field_count);
  info.position_index = find_field_index(fields, "position"_view, 0);
  info.half_size_index = find_field_index(fields, "half_size"_view, 1);
  info.alpha_index = find_field_index(fields, "alpha"_view, 2);
  info.valid = True;
  return info;
}

static auto push_fields(const PushInfo& push)
    -> View::Vector<Dialect::Shader::HostField> {
  if (!push.valid) {
    return View::Vector<Dialect::Shader::HostField>();
  }

  const auto& block = push.metadata.blocks.get_view()[push.block_index];
  return push.metadata.fields.get_view().slice(
      block.field_start, block.field_count);
}

static auto emit_push_names(
    spirv& assembler,
    Bits_32 push_type_id,
    Bits_32 push_variable_id,
    const PushInfo& push) -> void {
  if (!push.valid) {
    return;
  }

  const auto& block = push.metadata.blocks.get_view()[push.block_index];
  assembler.name(push_type_id, block.name);
  assembler.name(push_variable_id, "push"_view);

  View::Vector<Dialect::Shader::HostField> fields = push_fields(push);
  for (Count i = 0; i < fields.get_size(); i++) {
    assembler.member_name(push_type_id, Bits_32(i), fields[i].name);
  }
}

static auto emit_push_decorations(
    spirv& assembler,
    Bits_32 push_type_id,
    const PushInfo& push) -> void {
  assembler.decorate(push_type_id, spirv::Decoration::Block);

  View::Vector<Dialect::Shader::HostField> fields = push_fields(push);
  for (Count i = 0; i < fields.get_size(); i++) {
    assembler.member_decorate(
        push_type_id, Bits_32(i), spirv::Decoration::Offset,
        Bits_32(fields[i].offset));
  }
}

static auto push_member_type_id(
    View::Bytes type_name,
    Bits_32 real_32_type_id,
    Bits_32 vec_2d_type_id,
    Bits_32 vec_4d_type_id) -> Bits_32 {
  if (type_name == "Vec2D"_view) {
    return vec_2d_type_id;
  }
  if (type_name == "Vec4D"_view || type_name == "Color"_view) {
    return vec_4d_type_id;
  }
  return real_32_type_id;
}

static auto emit_push_type(
    spirv& assembler,
    Bits_32 push_type_id,
    Bits_32 real_32_type_id,
    Bits_32 vec_2d_type_id,
    Bits_32 vec_4d_type_id,
    const PushInfo& push) -> void {
  Static::Vector<Bits_32, 8> members;
  Count member_count = 0;

  View::Vector<Dialect::Shader::HostField> fields = push_fields(push);
  for (Count i = 0; i < fields.get_size() && i < 8; i++) {
    members[member_count++] = push_member_type_id(
        fields[i].type_name, real_32_type_id, vec_2d_type_id, vec_4d_type_id);
  }

  if (member_count == 0) {
    members[member_count++] = vec_2d_type_id;
    members[member_count++] = vec_2d_type_id;
    members[member_count++] = real_32_type_id;
  }

  assembler.type_struct(push_type_id, members.slice(0, member_count));
}

static auto emit_float_constant(
    spirv& assembler,
    Bits_32 real_32_type_id,
    Bits_32 result_id,
    Real_32 value) -> void {
  assembler.constant(real_32_type_id, result_id, real32_bits(value));
}

static auto emit_vec2_constants(
    spirv& assembler,
    Bits_32 real_32_type_id,
    Bits_32 vec_2d_type_id,
    View::Vector<Vec2Literal> values,
    Static::Vector<Bits_32, quad_vertex_count>& x_ids,
    Static::Vector<Bits_32, quad_vertex_count>& y_ids,
    Static::Vector<Bits_32, quad_vertex_count>& vec_ids) -> void {
  for (Count i = 0; i < quad_vertex_count; i++) {
    emit_float_constant(assembler, real_32_type_id, x_ids[i], values[i].x);
    emit_float_constant(assembler, real_32_type_id, y_ids[i], values[i].y);
    Static::Vector<Bits_32, 2> constituents(x_ids[i], y_ids[i]);
    assembler.constant_composite(vec_2d_type_id, vec_ids[i], constituents);
  }
}

static auto execution_model(
    Dialect::Shader::Stage stage,
    spirv::ExecutionModel* out) -> Bool {
  if (!out) {
    return False;
  }

  switch (stage) {
  case Dialect::Shader::Stage::Vertex:
    *out = spirv::ExecutionModel::Vertex;
    return True;
  case Dialect::Shader::Stage::Pixel:
    *out = spirv::ExecutionModel::Fragment;
    return True;
  case Dialect::Shader::Stage::None:
    return False;
  }

  return False;
}

static auto emit_v1_vertex_module(
    const Resolution::Record& record,
    const Dialect::Shader::EntryPoint& entry,
    Dynamic::Bytes& out) -> Bool {
  if (!entry.type || !entry.function ||
      entry.stage != Dialect::Shader::Stage::Vertex) {
    return False;
  }

  Dialect::Shader::Facts shader;
  Dialect::Shader::Interface interface = shader.interface_of(*entry.function);
  const Syntax::Ast::Member* vertex_index =
      find_member_with_builtin(*entry.type, shader, "vertex_index"_view);
  const Dialect::Shader::InterfaceField* uv_output =
      find_interface_field(interface.outputs.get_view(), "frag_uv"_view);
  if (!uv_output) {
    uv_output = first_user_output(interface.outputs.get_view());
  }

  QuadMesh quad = QuadMesh::from_type(*entry.type);
  PushInfo push = load_push_info(record);

  IdPool ids;
  const Bits_32 main_id = ids.take();
  const Bits_32 void_type_id = ids.take();
  const Bits_32 function_type_id = ids.take();
  const Bits_32 label_id = ids.take();
  const Bits_32 real_32_type_id = ids.take();
  const Bits_32 uint_32_type_id = ids.take();
  const Bits_32 signed_32_type_id = ids.take();
  const Bits_32 vec_2d_type_id = ids.take();
  const Bits_32 vec_4d_type_id = ids.take();
  const Bits_32 vec2_array_type_id = ids.take();
  const Bits_32 vec2_array_pointer_type_id = ids.take();
  const Bits_32 vec2_function_pointer_type_id = ids.take();
  const Bits_32 vec2_input_pointer_type_id = ids.take();
  const Bits_32 vec2_output_pointer_type_id = ids.take();
  const Bits_32 vec4_output_pointer_type_id = ids.take();
  const Bits_32 int_input_pointer_type_id = ids.take();
  const Bits_32 push_type_id = ids.take();
  const Bits_32 push_pointer_type_id = ids.take();
  const Bits_32 push_vec2_pointer_type_id = ids.take();
  const Bits_32 vertex_index_id = ids.take();
  const Bits_32 frag_uv_id = ids.take();
  const Bits_32 clip_position_id = ids.take();
  const Bits_32 push_id = ids.take();
  const Bits_32 array_length_id = ids.take();
  const Bits_32 index_zero_id = ids.take();
  const Bits_32 push_position_index_id = ids.take();
  const Bits_32 push_half_size_index_id = ids.take();
  const Bits_32 zero_float_id = ids.take();
  const Bits_32 one_float_id = ids.take();
  Static::Vector<Bits_32, quad_vertex_count> position_x_ids;
  Static::Vector<Bits_32, quad_vertex_count> position_y_ids;
  Static::Vector<Bits_32, quad_vertex_count> position_vec_ids;
  Static::Vector<Bits_32, quad_vertex_count> uv_x_ids;
  Static::Vector<Bits_32, quad_vertex_count> uv_y_ids;
  Static::Vector<Bits_32, quad_vertex_count> uv_vec_ids;
  for (Count i = 0; i < quad_vertex_count; i++) {
    position_x_ids[i] = ids.take();
    position_y_ids[i] = ids.take();
    position_vec_ids[i] = ids.take();
    uv_x_ids[i] = ids.take();
    uv_y_ids[i] = ids.take();
    uv_vec_ids[i] = ids.take();
  }
  const Bits_32 positions_array_id = ids.take();
  const Bits_32 uvs_array_id = ids.take();
  const Bits_32 positions_variable_id = ids.take();
  const Bits_32 uvs_variable_id = ids.take();
  const Bits_32 quad_pos_pointer_id = ids.take();
  const Bits_32 quad_pos_id = ids.take();
  const Bits_32 quad_uv_pointer_id = ids.take();
  const Bits_32 loaded_uv_id = ids.take();
  const Bits_32 loaded_vertex_index_id = ids.take();
  const Bits_32 push_position_pointer_id = ids.take();
  const Bits_32 push_position_id = ids.take();
  const Bits_32 push_half_size_pointer_id = ids.take();
  const Bits_32 push_half_size_id = ids.take();
  const Bits_32 scaled_position_id = ids.take();
  const Bits_32 screen_position_id = ids.take();
  const Bits_32 screen_x_id = ids.take();
  const Bits_32 screen_y_id = ids.take();
  const Bits_32 clip_value_id = ids.take();

  spirv assembler(out);
  assembler.begin_module(id_bound);
  assembler.capability(spirv::Capability::Shader);
  assembler.memory_model(
      spirv::AddressingModel::Logical, spirv::MemoryModel::GLSL450);

  Static::Vector<Bits_32, 3> interfaces(
      vertex_index_id, frag_uv_id, clip_position_id);
  assembler.entry_point(
      spirv::ExecutionModel::Vertex, main_id,
      entry.function->get_definition().get_name(), interfaces);

  assembler.name(main_id, entry.function->get_definition().get_name());
  if (vertex_index) {
    assembler.name(vertex_index_id, vertex_index->get_definition().get_name());
  } else {
    assembler.name(vertex_index_id, "vertex_index"_view);
  }
  assembler.name(frag_uv_id, uv_output ? uv_output->name : "frag_uv"_view);
  assembler.name(clip_position_id, "clip_position"_view);
  assembler.name(positions_variable_id, "quad_positions"_view);
  assembler.name(uvs_variable_id, "quad_uvs"_view);
  emit_push_names(assembler, push_type_id, push_id, push);

  assembler.decorate(
      vertex_index_id, spirv::Decoration::BuiltIn,
      Bits_32(spirv::BuiltIn::VertexIndex));
  assembler.decorate(
      frag_uv_id, spirv::Decoration::Location,
      Bits_32(uv_output ? uv_output->location : 0));
  assembler.decorate(
      clip_position_id, spirv::Decoration::BuiltIn,
      Bits_32(spirv::BuiltIn::Position));
  emit_push_decorations(assembler, push_type_id, push);

  assembler.type_void(void_type_id);
  assembler.type_function(function_type_id, void_type_id);
  assembler.type_float(real_32_type_id, 32);
  assembler.type_int(uint_32_type_id, 32, False);
  assembler.type_int(signed_32_type_id, 32, True);
  assembler.type_vector(vec_2d_type_id, real_32_type_id, 2);
  assembler.type_vector(vec_4d_type_id, real_32_type_id, 4);
  assembler.constant(
      uint_32_type_id, array_length_id, Bits_32(quad_vertex_count));
  assembler.type_array(vec2_array_type_id, vec_2d_type_id, array_length_id);
  assembler.type_pointer(
      vec2_array_pointer_type_id, spirv::StorageClass::Function,
      vec2_array_type_id);
  assembler.type_pointer(
      vec2_function_pointer_type_id, spirv::StorageClass::Function,
      vec_2d_type_id);
  assembler.type_pointer(
      vec2_input_pointer_type_id, spirv::StorageClass::Input, vec_2d_type_id);
  assembler.type_pointer(
      vec2_output_pointer_type_id, spirv::StorageClass::Output, vec_2d_type_id);
  assembler.type_pointer(
      vec4_output_pointer_type_id, spirv::StorageClass::Output, vec_4d_type_id);
  assembler.type_pointer(
      int_input_pointer_type_id, spirv::StorageClass::Input, signed_32_type_id);
  emit_push_type(
      assembler, push_type_id, real_32_type_id, vec_2d_type_id, vec_4d_type_id,
      push);
  assembler.type_pointer(
      push_pointer_type_id, spirv::StorageClass::PushConstant, push_type_id);
  assembler.type_pointer(
      push_vec2_pointer_type_id, spirv::StorageClass::PushConstant,
      vec_2d_type_id);

  assembler.constant(signed_32_type_id, index_zero_id, 0);
  assembler.constant(
      signed_32_type_id, push_position_index_id, Bits_32(push.position_index));
  assembler.constant(
      signed_32_type_id, push_half_size_index_id,
      Bits_32(push.half_size_index));
  emit_float_constant(assembler, real_32_type_id, zero_float_id, 0.0f);
  emit_float_constant(assembler, real_32_type_id, one_float_id, 1.0f);
  emit_vec2_constants(
      assembler, real_32_type_id, vec_2d_type_id, quad.positions,
      position_x_ids, position_y_ids, position_vec_ids);
  emit_vec2_constants(
      assembler, real_32_type_id, vec_2d_type_id, quad.uvs, uv_x_ids, uv_y_ids,
      uv_vec_ids);
  assembler.constant_composite(
      vec2_array_type_id, positions_array_id, position_vec_ids);
  assembler.constant_composite(vec2_array_type_id, uvs_array_id, uv_vec_ids);

  assembler.variable(
      int_input_pointer_type_id, vertex_index_id, spirv::StorageClass::Input);
  assembler.variable(
      vec2_output_pointer_type_id, frag_uv_id, spirv::StorageClass::Output);
  assembler.variable(
      vec4_output_pointer_type_id, clip_position_id,
      spirv::StorageClass::Output);
  assembler.variable(
      push_pointer_type_id, push_id, spirv::StorageClass::PushConstant);

  assembler.function(
      void_type_id, main_id, spirv::FunctionControl::None, function_type_id);
  assembler.label(label_id);
  assembler.variable(
      vec2_array_pointer_type_id, positions_variable_id,
      spirv::StorageClass::Function);
  assembler.variable(
      vec2_array_pointer_type_id, uvs_variable_id,
      spirv::StorageClass::Function);
  assembler.store(positions_variable_id, positions_array_id);
  assembler.store(uvs_variable_id, uvs_array_id);
  assembler.load(signed_32_type_id, loaded_vertex_index_id, vertex_index_id);

  Static::Vector<Bits_32, 1> array_indexes(loaded_vertex_index_id);
  assembler.access_chain(
      vec2_function_pointer_type_id, quad_pos_pointer_id, positions_variable_id,
      array_indexes);
  assembler.load(vec_2d_type_id, quad_pos_id, quad_pos_pointer_id);
  assembler.access_chain(
      vec2_function_pointer_type_id, quad_uv_pointer_id, uvs_variable_id,
      array_indexes);
  assembler.load(vec_2d_type_id, loaded_uv_id, quad_uv_pointer_id);
  assembler.store(frag_uv_id, loaded_uv_id);

  Static::Vector<Bits_32, 1> push_position_indexes(push_position_index_id);
  assembler.access_chain(
      push_vec2_pointer_type_id, push_position_pointer_id, push_id,
      push_position_indexes);
  assembler.load(vec_2d_type_id, push_position_id, push_position_pointer_id);

  Static::Vector<Bits_32, 1> push_half_size_indexes(push_half_size_index_id);
  assembler.access_chain(
      push_vec2_pointer_type_id, push_half_size_pointer_id, push_id,
      push_half_size_indexes);
  assembler.load(vec_2d_type_id, push_half_size_id, push_half_size_pointer_id);
  assembler.fmul(
      vec_2d_type_id, scaled_position_id, quad_pos_id, push_half_size_id);
  assembler.fadd(
      vec_2d_type_id, screen_position_id, push_position_id, scaled_position_id);

  Static::Vector<Bits_32, 1> x_index(0);
  Static::Vector<Bits_32, 1> y_index(1);
  assembler.composite_extract(
      real_32_type_id, screen_x_id, screen_position_id, x_index);
  assembler.composite_extract(
      real_32_type_id, screen_y_id, screen_position_id, y_index);
  Static::Vector<Bits_32, 4> clip_parts(
      screen_x_id, screen_y_id, zero_float_id, one_float_id);
  assembler.composite_construct(vec_4d_type_id, clip_value_id, clip_parts);
  assembler.store(clip_position_id, clip_value_id);

  assembler.return_void();
  assembler.function_end();

  return spirv::is_valid_module(out);
}

static auto emit_v1_pixel_module(
    const Resolution::Record& record,
    const Dialect::Shader::EntryPoint& entry,
    Dynamic::Bytes& out) -> Bool {
  if (!entry.type || !entry.function ||
      entry.stage != Dialect::Shader::Stage::Pixel) {
    return False;
  }

  Dialect::Shader::Facts shader;
  Dialect::Shader::Interface interface = shader.interface_of(*entry.function);
  Dynamic::Vector<Dialect::Shader::DescriptorBinding> descriptors =
      shader.descriptors(record);
  if (descriptors.get_size() == 0) {
    return False;
  }

  const Dialect::Shader::InterfaceField* uv_input =
      find_interface_field(interface.inputs.get_view(), "frag_uv"_view);
  if (!uv_input && interface.inputs.get_size() > 0) {
    uv_input = &interface.inputs.get_view()[0];
  }

  PushInfo push = load_push_info(record);

  IdPool ids;
  const Bits_32 main_id = ids.take();
  const Bits_32 void_type_id = ids.take();
  const Bits_32 function_type_id = ids.take();
  const Bits_32 label_id = ids.take();
  const Bits_32 real_32_type_id = ids.take();
  const Bits_32 signed_32_type_id = ids.take();
  const Bits_32 vec_2d_type_id = ids.take();
  const Bits_32 vec_3d_type_id = ids.take();
  const Bits_32 vec_4d_type_id = ids.take();
  const Bits_32 image_type_id = ids.take();
  const Bits_32 sampled_image_type_id = ids.take();
  const Bits_32 sampled_image_pointer_type_id = ids.take();
  const Bits_32 vec2_input_pointer_type_id = ids.take();
  const Bits_32 vec4_output_pointer_type_id = ids.take();
  const Bits_32 push_type_id = ids.take();
  const Bits_32 push_pointer_type_id = ids.take();
  const Bits_32 push_float_pointer_type_id = ids.take();
  const Bits_32 texture_id = ids.take();
  const Bits_32 frag_uv_id = ids.take();
  const Bits_32 out_color_id = ids.take();
  const Bits_32 push_id = ids.take();
  const Bits_32 alpha_index_id = ids.take();
  const Bits_32 loaded_texture_id = ids.take();
  const Bits_32 loaded_uv_id = ids.take();
  const Bits_32 sample_id = ids.take();
  const Bits_32 rgb_id = ids.take();
  const Bits_32 sample_alpha_id = ids.take();
  const Bits_32 push_alpha_pointer_id = ids.take();
  const Bits_32 push_alpha_id = ids.take();
  const Bits_32 out_alpha_id = ids.take();
  const Bits_32 out_r_id = ids.take();
  const Bits_32 out_g_id = ids.take();
  const Bits_32 out_b_id = ids.take();
  const Bits_32 out_value_id = ids.take();

  spirv assembler(out);
  assembler.begin_module(id_bound);
  assembler.capability(spirv::Capability::Shader);
  assembler.memory_model(
      spirv::AddressingModel::Logical, spirv::MemoryModel::GLSL450);

  Static::Vector<Bits_32, 2> interfaces(frag_uv_id, out_color_id);
  assembler.entry_point(
      spirv::ExecutionModel::Fragment, main_id,
      entry.function->get_definition().get_name(), interfaces);
  assembler.execution_mode(main_id, spirv::ExecutionMode::OriginUpperLeft);

  assembler.name(main_id, entry.function->get_definition().get_name());
  assembler.name(texture_id, descriptors[0].name);
  assembler.name(frag_uv_id, uv_input ? uv_input->name : "frag_uv"_view);
  assembler.name(out_color_id, "out_color"_view);
  emit_push_names(assembler, push_type_id, push_id, push);

  assembler.decorate(
      texture_id, spirv::Decoration::DescriptorSet,
      Bits_32(descriptors[0].set));
  assembler.decorate(
      texture_id, spirv::Decoration::Binding, Bits_32(descriptors[0].slot));
  assembler.decorate(
      frag_uv_id, spirv::Decoration::Location,
      Bits_32(uv_input ? uv_input->location : 0));
  assembler.decorate(out_color_id, spirv::Decoration::Location, 0);
  emit_push_decorations(assembler, push_type_id, push);

  assembler.type_void(void_type_id);
  assembler.type_function(function_type_id, void_type_id);
  assembler.type_float(real_32_type_id, 32);
  assembler.type_int(signed_32_type_id, 32, True);
  assembler.type_vector(vec_2d_type_id, real_32_type_id, 2);
  assembler.type_vector(vec_3d_type_id, real_32_type_id, 3);
  assembler.type_vector(vec_4d_type_id, real_32_type_id, 4);
  assembler.type_image(
      image_type_id, real_32_type_id, spirv::Dim::D2, 0, 0, 0, 1,
      spirv::ImageFormat::Unknown);
  assembler.type_sampled_image(sampled_image_type_id, image_type_id);
  assembler.type_pointer(
      sampled_image_pointer_type_id, spirv::StorageClass::UniformConstant,
      sampled_image_type_id);
  assembler.type_pointer(
      vec2_input_pointer_type_id, spirv::StorageClass::Input, vec_2d_type_id);
  assembler.type_pointer(
      vec4_output_pointer_type_id, spirv::StorageClass::Output, vec_4d_type_id);
  emit_push_type(
      assembler, push_type_id, real_32_type_id, vec_2d_type_id, vec_4d_type_id,
      push);
  assembler.type_pointer(
      push_pointer_type_id, spirv::StorageClass::PushConstant, push_type_id);
  assembler.type_pointer(
      push_float_pointer_type_id, spirv::StorageClass::PushConstant,
      real_32_type_id);

  assembler.constant(
      signed_32_type_id, alpha_index_id, Bits_32(push.alpha_index));

  assembler.variable(
      sampled_image_pointer_type_id, texture_id,
      spirv::StorageClass::UniformConstant);
  assembler.variable(
      vec2_input_pointer_type_id, frag_uv_id, spirv::StorageClass::Input);
  assembler.variable(
      vec4_output_pointer_type_id, out_color_id, spirv::StorageClass::Output);
  assembler.variable(
      push_pointer_type_id, push_id, spirv::StorageClass::PushConstant);

  assembler.function(
      void_type_id, main_id, spirv::FunctionControl::None, function_type_id);
  assembler.label(label_id);
  assembler.load(sampled_image_type_id, loaded_texture_id, texture_id);
  assembler.load(vec_2d_type_id, loaded_uv_id, frag_uv_id);
  assembler.image_sample_implicit_lod(
      vec_4d_type_id, sample_id, loaded_texture_id, loaded_uv_id);
  Static::Vector<Bits_32, 3> rgb_components(0, 1, 2);
  assembler.vector_shuffle(
      vec_3d_type_id, rgb_id, sample_id, sample_id, rgb_components);
  Static::Vector<Bits_32, 1> alpha_component(3);
  assembler.composite_extract(
      real_32_type_id, sample_alpha_id, sample_id, alpha_component);
  Static::Vector<Bits_32, 1> push_alpha_indexes(alpha_index_id);
  assembler.access_chain(
      push_float_pointer_type_id, push_alpha_pointer_id, push_id,
      push_alpha_indexes);
  assembler.load(real_32_type_id, push_alpha_id, push_alpha_pointer_id);
  assembler.fmul(real_32_type_id, out_alpha_id, sample_alpha_id, push_alpha_id);

  Static::Vector<Bits_32, 1> r_component(0);
  Static::Vector<Bits_32, 1> g_component(1);
  Static::Vector<Bits_32, 1> b_component(2);
  assembler.composite_extract(real_32_type_id, out_r_id, rgb_id, r_component);
  assembler.composite_extract(real_32_type_id, out_g_id, rgb_id, g_component);
  assembler.composite_extract(real_32_type_id, out_b_id, rgb_id, b_component);
  Static::Vector<Bits_32, 4> out_parts(
      out_r_id, out_g_id, out_b_id, out_alpha_id);
  assembler.composite_construct(vec_4d_type_id, out_value_id, out_parts);
  assembler.store(out_color_id, out_value_id);
  assembler.return_void();
  assembler.function_end();

  return spirv::is_valid_module(out);
}

auto Tetrodotoxin::Compiler::Shader::emit_v1_stage_module(
    const Resolution::Record& record,
    const Dialect::Shader::EntryPoint& entry,
    Dynamic::Bytes& out) -> Bool {
  out.clear();

  spirv::ExecutionModel model = spirv::ExecutionModel::Vertex;
  if (!execution_model(entry.stage, &model)) {
    return False;
  }

  switch (entry.stage) {
  case Dialect::Shader::Stage::Vertex:
    return emit_v1_vertex_module(record, entry, out);
  case Dialect::Shader::Stage::Pixel:
    return emit_v1_pixel_module(record, entry, out);
  case Dialect::Shader::Stage::None:
    return False;
  }

  return False;
}

auto Tetrodotoxin::Compiler::Shader::emit_v1_modules(
    const Resolution::Record& record,
    Dynamic::Bytes& vertex,
    Dynamic::Bytes& pixel) -> Bool {
  vertex.clear();
  pixel.clear();

  Dialect::Shader::Facts shader;
  Dynamic::Vector<Dialect::Shader::EntryPoint> entries =
      shader.entry_points(record);

  Bool emitted_vertex = False;
  Bool emitted_pixel = False;
  for (Count i = 0; i < entries.get_size(); i++) {
    switch (entries[i].stage) {
    case Dialect::Shader::Stage::Vertex:
      if (emitted_vertex || !emit_v1_stage_module(record, entries[i], vertex)) {
        return False;
      }
      emitted_vertex = True;
      break;
    case Dialect::Shader::Stage::Pixel:
      if (emitted_pixel || !emit_v1_stage_module(record, entries[i], pixel)) {
        return False;
      }
      emitted_pixel = True;
      break;
    case Dialect::Shader::Stage::None:
      break;
    }
  }

  return emitted_vertex && emitted_pixel;
}
