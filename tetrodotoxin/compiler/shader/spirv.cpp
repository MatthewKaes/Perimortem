// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/compiler/shader/spirv.hpp"

#include "perimortem/core/diagnostics/log.hpp"
#include "perimortem/core/reader/textual.hpp"
#include "perimortem/core/static/vector.hpp"
#include "perimortem/core/data.hpp"

#include "tetrodotoxin/compiler/assembler/spirv.hpp"
#include "tetrodotoxin/dialect/shader/quad.hpp"
#include "tetrodotoxin/syntax/ast/statement.hpp"
#include "tetrodotoxin/syntax/pack.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Tetrodotoxin;
using namespace Tetrodotoxin::Compiler::Assembler;
using namespace Tetrodotoxin::Compiler::Shader;

using QuadMesh = Dialect::Shader::QuadMesh;
using Vec2Literal = Dialect::Shader::QuadMesh::Vec2;

static constexpr Bits_32 id_bound = 256;
static constexpr Count quad_vertex_count = QuadMesh::vertex_count;

class IdPool {
 public:
  auto take() -> Bits_32 { return next++; }

 private:
  Bits_32 next = 1;
};

struct PushInfo {
  Dialect::Shader::HostInputMetadata metadata;
  Bool valid = False;
};

enum class ShaderValueKind : Bits_8 {
  Invalid,
  Real32,
  Signed32,
  Vec2,
  Vec3,
  Vec4,
  SampledImage,
  Vec2Array,
};

struct ShaderValue {
  Bits_32 id = 0;
  ShaderValueKind kind = ShaderValueKind::Invalid;
  Bool is_pointer = False;
  spirv::StorageClass storage = spirv::StorageClass::Function;
};

struct NamedShaderValue {
  View::Bytes name;
  ShaderValue value;
};

struct StageVariable {
  View::Bytes name;
  ShaderValue value;
  Dialect::Shader::Builtin builtin;
  Count location = 0;
  Count descriptor_set = 0;
  Count descriptor_slot = 0;
  Bool descriptor = False;
};

struct FloatConstant {
  Bits_32 bits = 0;
  Bits_32 id = 0;
};

struct PushFieldValue {
  View::Bytes name;
  ShaderValueKind kind = ShaderValueKind::Invalid;
  Bits_32 index_id = 0;
};

struct Vec2ArrayValue {
  View::Bytes name;
  Static::Vector<Vec2Literal, quad_vertex_count> values;
  Count value_count = 0;
  Static::Vector<Bits_32, quad_vertex_count> x_component_ids;
  Static::Vector<Bits_32, quad_vertex_count> y_component_ids;
  Static::Vector<Bits_32, quad_vertex_count> vector_ids;
  Bits_32 constant_id = 0;
  Bits_32 variable_id = 0;
};

struct ShaderTypes {
  Bits_32 void_type_id = 0;
  Bits_32 function_type_id = 0;
  Bits_32 real_32_type_id = 0;
  Bits_32 uint_32_type_id = 0;
  Bits_32 signed_32_type_id = 0;
  Bits_32 vec_2d_type_id = 0;
  Bits_32 vec_3d_type_id = 0;
  Bits_32 vec_4d_type_id = 0;
  Bits_32 vec2_array_type_id = 0;
  Bits_32 vec2_array_pointer_type_id = 0;
  Bits_32 vec2_function_pointer_type_id = 0;
  Bits_32 image_type_id = 0;
  Bits_32 sampled_image_type_id = 0;
  Bits_32 sampled_image_pointer_type_id = 0;
  Bits_32 vec2_input_pointer_type_id = 0;
  Bits_32 vec2_output_pointer_type_id = 0;
  Bits_32 vec4_output_pointer_type_id = 0;
  Bits_32 int_input_pointer_type_id = 0;
  Bits_32 push_type_id = 0;
  Bits_32 push_pointer_type_id = 0;
  Bits_32 push_float_pointer_type_id = 0;
  Bits_32 push_vec2_pointer_type_id = 0;
  Bits_32 push_vec4_pointer_type_id = 0;
};

static auto real32_bits(Real_32 value) -> Bits_32 {
  Bits_32 output = 0;
  Data::copy(Data::cast<Bits_8>(&output), &value, 1);
  return output;
}

static auto parse_real_literal(View::Bytes text, Real_32* parsed_real)
    -> Bool {
  if (!parsed_real || text.is_empty()) {
    return False;
  }

  Reader::Textual reader(text);
  Real_32 result = reader.read_real_32();
  if (!reader.is_valid() || !reader.is_empty()) {
    return False;
  }

  *parsed_real = result;
  return True;
}

static auto expression_name(
    const Syntax::Expression::Expression& expression,
    View::Bytes* output) -> Bool {
  if (!output) {
    return False;
  }

  switch (expression.get_class().get_type()) {
  case ::Ttx::Lexical::Class::Type::Self:
    *output = "self"_view;
    return True;

  case ::Ttx::Lexical::Class::Type::Addressable:
    *output = expression.get_text();
    return True;

  case ::Ttx::Lexical::Class::Type::Type:
    *output = expression.get_type_ref().get_name();
    return True;

  default:
    return False;
  }
}

static auto component_index(View::Bytes component, Bits_32* output) -> Bool {
  if (!output) {
    return False;
  }

  if (component == "r"_view || component == "x"_view) {
    *output = 0;
    return True;
  }
  if (component == "width"_view) {
    *output = 0;
    return True;
  }
  if (component == "g"_view || component == "y"_view) {
    *output = 1;
    return True;
  }
  if (component == "height"_view) {
    *output = 1;
    return True;
  }
  if (component == "b"_view || component == "z"_view) {
    *output = 2;
    return True;
  }
  if (component == "a"_view || component == "w"_view) {
    *output = 3;
    return True;
  }

  return False;
}

static auto value_component_count(ShaderValueKind kind) -> Count {
  switch (kind) {
  case ShaderValueKind::Real32:
  case ShaderValueKind::Signed32:
  case ShaderValueKind::SampledImage:
  case ShaderValueKind::Vec2Array:
    return 1;
  case ShaderValueKind::Vec2:
    return 2;
  case ShaderValueKind::Vec3:
    return 3;
  case ShaderValueKind::Vec4:
    return 4;
  case ShaderValueKind::Invalid:
    return 0;
  }

  return 0;
}

static auto vector_kind(Count component_count) -> ShaderValueKind {
  switch (component_count) {
  case 1:
    return ShaderValueKind::Real32;
  case 2:
    return ShaderValueKind::Vec2;
  case 3:
    return ShaderValueKind::Vec3;
  case 4:
    return ShaderValueKind::Vec4;
  default:
    return ShaderValueKind::Invalid;
  }
}

static auto value_kind_from_type_name(View::Bytes type_name)
    -> ShaderValueKind {
  if (type_name == "Real_32"_view) {
    return ShaderValueKind::Real32;
  }
  if (type_name == "Bits_32"_view) {
    return ShaderValueKind::Signed32;
  }
  if (type_name == "Vec2D"_view) {
    return ShaderValueKind::Vec2;
  }
  if (type_name == "Size2D"_view) {
    return ShaderValueKind::Vec2;
  }
  if (type_name == "Vec3D"_view) {
    return ShaderValueKind::Vec3;
  }
  if (type_name == "Vec4D"_view || type_name == "Color"_view) {
    return ShaderValueKind::Vec4;
  }
  if (type_name == "Sampler2D"_view) {
    return ShaderValueKind::SampledImage;
  }

  return ShaderValueKind::Invalid;
}

static auto value_kind_from_type_ref(const Syntax::Type::Ref& type)
    -> ShaderValueKind {
  return value_kind_from_type_name(type.get_last_name());
}

static auto load_push_info(
    Resolution::Resolver& resolver,
    const Resolution::Record& record) -> PushInfo {
  Dialect::Shader::Metadata shader;
  PushInfo info;
  info.metadata = shader.host_inputs(resolver, record);
  for (Count field_index = 0; field_index < info.metadata.fields.get_size();
       field_index++) {
    Diagnostics::Log::Message<512> debug(Diagnostics::Log::Level::Error);
    debug << "Shader push field "_view << info.metadata.fields[field_index].name;
  }
  if (info.metadata.fields.get_size() == 0) {
    return info;
  }

  info.valid = True;
  return info;
}

static auto push_fields(const PushInfo& push)
    -> View::Vector<Dialect::Shader::HostField> {
  if (!push.valid) {
    return View::Vector<Dialect::Shader::HostField>();
  }

  return push.metadata.fields.get_view();
}

static auto emit_push_names(
    spirv& assembler,
    Bits_32 push_type_id,
    Bits_32 push_variable_id,
    const PushInfo& push) -> void {
  if (!push.valid) {
    return;
  }

  assembler.name(push_type_id, "HostInputs"_view);
  assembler.name(push_variable_id, "host_inputs"_view);

  View::Vector<Dialect::Shader::HostField> fields = push_fields(push);
  for (Count i = 0; i < fields.get_size(); i++) {
    assembler.member_name(push_type_id, Bits_32(i), fields[i].name);
  }
}

static auto emit_push_decorations(
    spirv& assembler,
    Bits_32 push_type_id,
    const PushInfo& push) -> void {
  if (!push.valid) {
    return;
  }

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
  if (type_name == "Size2D"_view) {
    return vec_2d_type_id;
  }
  if (type_name == "Vec4D"_view || type_name == "Color"_view) {
    return vec_4d_type_id;
  }
  if (type_name == "Vec3D"_view) {
    return real_32_type_id;
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
    Static::Vector<Bits_32, quad_vertex_count>& x_component_ids,
    Static::Vector<Bits_32, quad_vertex_count>& y_component_ids,
    Static::Vector<Bits_32, quad_vertex_count>& vector_ids) -> void {
  for (Count i = 0; i < quad_vertex_count; i++) {
    emit_float_constant(
        assembler, real_32_type_id, x_component_ids[i], values[i].x);
    emit_float_constant(
        assembler, real_32_type_id, y_component_ids[i], values[i].y);
    Static::Vector<Bits_32, 2> constituents(
        x_component_ids[i], y_component_ids[i]);
    assembler.constant_composite(vec_2d_type_id, vector_ids[i], constituents);
  }
}

static auto type_id_for(
    const ShaderTypes& types,
    ShaderValueKind kind) -> Bits_32 {
  switch (kind) {
  case ShaderValueKind::Real32:
    return types.real_32_type_id;
  case ShaderValueKind::Signed32:
    return types.signed_32_type_id;
  case ShaderValueKind::Vec2:
    return types.vec_2d_type_id;
  case ShaderValueKind::Vec3:
    return types.vec_3d_type_id;
  case ShaderValueKind::Vec4:
    return types.vec_4d_type_id;
  case ShaderValueKind::SampledImage:
    return types.sampled_image_type_id;
  case ShaderValueKind::Vec2Array:
    return types.vec2_array_type_id;
  case ShaderValueKind::Invalid:
    return 0;
  }

  return 0;
}

static auto pointer_type_id_for(
    const ShaderTypes& types,
    ShaderValueKind kind,
    spirv::StorageClass storage) -> Bits_32 {
  switch (storage) {
  case spirv::StorageClass::Input:
    if (kind == ShaderValueKind::Vec2) {
      return types.vec2_input_pointer_type_id;
    }
    if (kind == ShaderValueKind::Signed32) {
      return types.int_input_pointer_type_id;
    }
    return 0;

  case spirv::StorageClass::Output:
    if (kind == ShaderValueKind::Vec2) {
      return types.vec2_output_pointer_type_id;
    }
    if (kind == ShaderValueKind::Vec4) {
      return types.vec4_output_pointer_type_id;
    }
    return 0;

  case spirv::StorageClass::Function:
    if (kind == ShaderValueKind::Vec2Array) {
      return types.vec2_array_pointer_type_id;
    }
    if (kind == ShaderValueKind::Vec2) {
      return types.vec2_function_pointer_type_id;
    }
    return 0;

  case spirv::StorageClass::PushConstant:
    if (kind == ShaderValueKind::Real32) {
      return types.push_float_pointer_type_id;
    }
    if (kind == ShaderValueKind::Vec2) {
      return types.push_vec2_pointer_type_id;
    }
    if (kind == ShaderValueKind::Vec4) {
      return types.push_vec4_pointer_type_id;
    }
    return 0;

  case spirv::StorageClass::UniformConstant:
    if (kind == ShaderValueKind::SampledImage) {
      return types.sampled_image_pointer_type_id;
    }
    return 0;

  case spirv::StorageClass::Uniform:
    return 0;
  }

  return 0;
}

static auto find_float_constant(
    View::Vector<FloatConstant> constants,
    Bits_32 bits,
    Bits_32* id) -> Bool {
  for (Count i = 0; i < constants.get_size(); i++) {
    if (constants[i].bits == bits) {
      if (id) {
        *id = constants[i].id;
      }
      return True;
    }
  }

  return False;
}

static auto insert_float_constant(
    Dynamic::Vector<FloatConstant>& constants,
    IdPool& id_pool,
    Bits_32 bits) -> void {
  Bits_32 existing_id = 0;
  if (find_float_constant(constants.get_view(), bits, &existing_id)) {
    return;
  }

  constants.insert({bits, id_pool.take()});
}

static auto constant_bits_from_expression(
    const Syntax::Expression::Expression* expression,
    Bits_32* bits) -> Bool {
  if (!expression || !bits) {
    return False;
  }

  switch (expression->get_class().get_type()) {
  case ::Ttx::Lexical::Class::Type::Float:
  case ::Ttx::Lexical::Class::Type::Numeric: {
    Real_32 value = 0.0f;
    if (!parse_real_literal(expression->get_text(), &value)) {
      return False;
    }

    *bits = real32_bits(value);
    return True;
  }

  case ::Ttx::Lexical::Class::Type::SubOp: {
    if (expression->get_left() || !expression->get_right()) {
      return False;
    }

    Real_32 value = 0.0f;
    if (!parse_real_literal(expression->get_right()->get_text(), &value)) {
      return False;
    }

    *bits = real32_bits(-value);
    return True;
  }

  default:
    return False;
  }
}

static auto collect_expression_float_constants(
    const Syntax::Expression::Expression* expression,
    Dynamic::Vector<FloatConstant>& constants,
    IdPool& id_pool) -> void {
  if (!expression) {
    return;
  }

  Bits_32 bits = 0;
  if (constant_bits_from_expression(expression, &bits)) {
    insert_float_constant(constants, id_pool, bits);
  }

  collect_expression_float_constants(expression->get_left(), constants, id_pool);
  collect_expression_float_constants(expression->get_right(), constants, id_pool);

  View::Vector<Syntax::Expression::Expression*> arguments =
      expression->get_args();
  for (Count i = 0; i < arguments.get_size(); i++) {
    collect_expression_float_constants(arguments[i], constants, id_pool);
  }

  if (expression->get_kind() == Syntax::Expression::Kind::Pack) {
    const auto* pack = static_cast<const Syntax::Pack*>(expression);
    View::Vector<Syntax::Pack::Field> fields = pack->get_fields();
    for (Count i = 0; i < fields.get_size(); i++) {
      collect_expression_float_constants(fields[i].value, constants, id_pool);
    }
  }
}

static auto collect_body_float_constants(
    View::Vector<Syntax::Ast::Statement*> body,
    Dynamic::Vector<FloatConstant>& constants,
    IdPool& id_pool) -> void {
  for (Count i = 0; i < body.get_size(); i++) {
    if (!body[i]) {
      continue;
    }

    collect_expression_float_constants(body[i]->get_value(), constants, id_pool);
    collect_expression_float_constants(body[i]->get_cond(), constants, id_pool);
  }
}

class ShaderBodyEmitter {
 public:
  ShaderBodyEmitter(
      spirv& assembler,
      IdPool& id_pool,
      const ShaderTypes& types,
      View::Vector<FloatConstant> float_constants,
      View::Vector<NamedShaderValue> self_fields,
      View::Vector<NamedShaderValue> uniform_fields,
      View::Vector<NamedShaderValue> output_fields)
      : assembler(assembler),
        id_pool(id_pool),
        types(types),
        float_constants(float_constants),
        self_fields(self_fields),
        uniform_fields(uniform_fields),
        output_fields(output_fields) {}

  auto add_symbol(View::Bytes name, ShaderValue value) -> void {
    symbols.insert({name, value});
  }

  auto emit_body(View::Vector<Syntax::Ast::Statement*> body) -> Bool {
    for (Count i = 0; i < body.get_size(); i++) {
      if (!body[i]) {
        continue;
      }

      if (body[i]->is_declaration()) {
        ShaderValue value;
        if (!emit_expression(body[i]->get_value(), &value)) {
          Diagnostics::Log::Message<512> error(Diagnostics::Log::Level::Error);
          error << "Shader emitter failed to lower declaration "_view
                << body[i]->get_name();
          return False;
        }

        symbols.insert({body[i]->get_name(), value});
        continue;
      }

      if (body[i]->get_class() == ::Ttx::Lexical::Class::Type::Return) {
        if (!emit_return(body[i]->get_value())) {
          Diagnostics::Log::Message<512> error(Diagnostics::Log::Level::Error);
          error << "Shader emitter failed to lower return expression"_view;
          return False;
        }

        return True;
      }

      Diagnostics::Log::Message<512> error(Diagnostics::Log::Level::Error);
      error << "Shader emitter encountered an unsupported statement"_view;
      return False;
    }

    return False;
  }

 private:
  auto find_symbol(View::Bytes name, ShaderValue* value) const -> Bool {
    for (Count i = 0; i < symbols.get_size(); i++) {
      if (symbols[i].name == name) {
        if (value) {
          *value = symbols[i].value;
        }
        return True;
      }
    }

    return False;
  }

  auto find_self_field(View::Bytes name, ShaderValue* value) const -> Bool {
    for (Count i = 0; i < self_fields.get_size(); i++) {
      if (self_fields[i].name == name) {
        if (value) {
          *value = self_fields[i].value;
        }
        return True;
      }
    }

    return False;
  }

  auto find_output_field(View::Bytes name, ShaderValue* value) const -> Bool {
    for (Count i = 0; i < output_fields.get_size(); i++) {
      if (output_fields[i].name == name) {
        if (value) {
          *value = output_fields[i].value;
        }
        return True;
      }
    }

    return False;
  }

  auto find_uniform_field(View::Bytes name, ShaderValue* value) const -> Bool {
    for (Count i = 0; i < uniform_fields.get_size(); i++) {
      if (uniform_fields[i].name == name) {
        if (value) {
          *value = uniform_fields[i].value;
        }
        return True;
      }
    }

    return False;
  }

  auto find_default_output(ShaderValue* value) const -> Bool {
    if (output_fields.is_empty()) {
      return False;
    }

    if (value) {
      *value = output_fields[0].value;
    }
    return True;
  }

  auto emit_load(ShaderValue source, ShaderValue* output) -> Bool {
    if (!output || source.kind == ShaderValueKind::Invalid) {
      return False;
    }

    if (!source.is_pointer) {
      *output = source;
      return True;
    }

    Bits_32 result_type_id = type_id_for(types, source.kind);
    if (result_type_id == 0) {
      return False;
    }

    ShaderValue result;
    result.id = id_pool.take();
    result.kind = source.kind;
    assembler.load(result_type_id, result.id, source.id);
    *output = result;
    return True;
  }

  auto emit_float_constant(
      const Syntax::Expression::Expression& expression,
      ShaderValue* output) -> Bool {
    Bits_32 bits = 0;
    Bits_32 constant_id = 0;
    if (!constant_bits_from_expression(&expression, &bits) ||
        !find_float_constant(float_constants, bits, &constant_id)) {
      return False;
    }

    *output = {constant_id, ShaderValueKind::Real32, False};
    return True;
  }

  auto emit_addressable(
      const Syntax::Expression::Expression& expression,
      ShaderValue* output) -> Bool {
    View::Bytes name;
    if (!expression_name(expression, &name)) {
      return False;
    }

    ShaderValue symbol;
    if (!find_symbol(name, &symbol)) {
      return False;
    }

    return emit_load(symbol, output);
  }

  auto emit_self_field(
      View::Bytes field_name,
      ShaderValue* output) -> Bool {
    ShaderValue field;
    if (!find_self_field(field_name, &field)) {
      return False;
    }

    if (field.kind == ShaderValueKind::SampledImage ||
        field.kind == ShaderValueKind::Vec2Array) {
      *output = field;
      return True;
    }

    return emit_load(field, output);
  }

  auto emit_uniform_field(
      View::Bytes field_name,
      ShaderValue* output) -> Bool {
    ShaderValue field;
    if (!find_uniform_field(field_name, &field)) {
      Diagnostics::Log::Message<512> error(Diagnostics::Log::Level::Error);
      error << "Shader emitter could not find uniform field "_view
            << field_name;
      return False;
    }

    if (field.kind == ShaderValueKind::SampledImage ||
        field.kind == ShaderValueKind::Vec2Array) {
      *output = field;
      return True;
    }

    return emit_load(field, output);
  }

  auto emit_vector_component(
      ShaderValue source,
      View::Bytes field_name,
      ShaderValue* output) -> Bool {
    ShaderValue value;
    if (!emit_load(source, &value)) {
      return False;
    }

    Count component_count = value_component_count(value.kind);
    Bits_32 index = 0;
    if (component_count < 2 || !component_index(field_name, &index) ||
        Count(index) >= component_count) {
      Diagnostics::Log::Message<512> error(Diagnostics::Log::Level::Error);
      error << "Shader emitter could not resolve vector component "_view
            << field_name;
      return False;
    }

    ShaderValue result;
    result.id = id_pool.take();
    result.kind = ShaderValueKind::Real32;
    Static::Vector<Bits_32, 1> indexes(index);
    assembler.composite_extract(
        types.real_32_type_id, result.id, value.id, indexes);
    *output = result;
    return True;
  }

  auto emit_field_access(
      const Syntax::Expression::Expression& expression,
      ShaderValue* output) -> Bool {
    if (!expression.get_left()) {
      return False;
    }

    View::Bytes base_name;
    if (expression_name(*expression.get_left(), &base_name)) {
      if (base_name == "self"_view) {
        return emit_self_field(expression.get_text(), output);
      }
      if (base_name == "uniform"_view) {
        return emit_uniform_field(expression.get_text(), output);
      }
    }

    ShaderValue base;
    if (!emit_expression(expression.get_left(), &base)) {
      return False;
    }

    return emit_vector_component(base, expression.get_text(), output);
  }

  auto emit_index(
      const Syntax::Expression::Expression& expression,
      ShaderValue* output) -> Bool {
    if (!expression.get_left() || !expression.get_right()) {
      return False;
    }

    ShaderValue base;
    ShaderValue index;
    if (!emit_expression(expression.get_left(), &base) ||
        !emit_expression(expression.get_right(), &index)) {
      return False;
    }

    if (!base.is_pointer || base.kind != ShaderValueKind::Vec2Array) {
      return False;
    }

    ShaderValue loaded_index;
    if (!emit_load(index, &loaded_index) ||
        loaded_index.kind != ShaderValueKind::Signed32) {
      return False;
    }

    ShaderValue element_ptr;
    element_ptr.id = id_pool.take();
    element_ptr.kind = ShaderValueKind::Vec2;
    element_ptr.is_pointer = True;
    element_ptr.storage = spirv::StorageClass::Function;
    Static::Vector<Bits_32, 1> indexes(loaded_index.id);
    assembler.access_chain(
        types.vec2_function_pointer_type_id, element_ptr.id, base.id,
        indexes);
    return emit_load(element_ptr, output);
  }

  auto emit_swizzle(
      const Syntax::Expression::Expression& expression,
      ShaderValue* output) -> Bool {
    if (!expression.get_left()) {
      return False;
    }

    ShaderValue base;
    if (!emit_expression(expression.get_left(), &base)) {
      return False;
    }

    ShaderValue source;
    if (!emit_load(base, &source)) {
      return False;
    }

    View::Vector<Syntax::Expression::Expression*> fields = expression.get_args();
    ShaderValueKind result_kind = vector_kind(fields.get_size());
    if (result_kind == ShaderValueKind::Invalid) {
      return False;
    }

    Static::Vector<Bits_32, 4> components;
    for (Count i = 0; i < fields.get_size(); i++) {
      if (!fields[i] ||
          !component_index(fields[i]->get_text(), &components[i]) ||
          Count(components[i]) >= value_component_count(source.kind)) {
        return False;
      }
    }

    ShaderValue result;
    result.id = id_pool.take();
    result.kind = result_kind;
    if (result_kind == ShaderValueKind::Real32) {
      Static::Vector<Bits_32, 1> indexes(components[0]);
      assembler.composite_extract(
          types.real_32_type_id, result.id, source.id, indexes);
    } else {
      assembler.vector_shuffle(
          type_id_for(types, result_kind), result.id, source.id, source.id,
          components.slice(0, fields.get_size()));
    }

    *output = result;
    return True;
  }

  auto emit_binary(
      const Syntax::Expression::Expression& expression,
      ShaderValue* output) -> Bool {
    if (!expression.get_left() || !expression.get_right()) {
      return False;
    }

    ShaderValue left;
    ShaderValue right;
    if (!emit_expression(expression.get_left(), &left) ||
        !emit_expression(expression.get_right(), &right) ||
        left.kind != right.kind) {
      return False;
    }

    ShaderValue result;
    result.id = id_pool.take();
    result.kind = left.kind;
    Bits_32 result_type_id = type_id_for(types, result.kind);
    if (result_type_id == 0) {
      return False;
    }

    switch (expression.get_class().get_type()) {
    case ::Ttx::Lexical::Class::Type::AddOp:
      assembler.fadd(result_type_id, result.id, left.id, right.id);
      break;

    case ::Ttx::Lexical::Class::Type::SubOp:
      assembler.fsub(result_type_id, result.id, left.id, right.id);
      break;

    case ::Ttx::Lexical::Class::Type::MulOp:
      assembler.fmul(result_type_id, result.id, left.id, right.id);
      break;

    default:
      return False;
    }

    *output = result;
    return True;
  }

  auto emit_call(
      const Syntax::Expression::Expression& expression,
      ShaderValue* output) -> Bool {
    if (expression.get_text() != "sample"_view || !expression.get_left() ||
        !expression.get_right()) {
      return False;
    }

    ShaderValue sampler_pointer;
    if (!emit_expression(expression.get_left(), &sampler_pointer) ||
        sampler_pointer.kind != ShaderValueKind::SampledImage) {
      return False;
    }

    ShaderValue sampler;
    if (!emit_load(sampler_pointer, &sampler)) {
      return False;
    }

    View::Vector<Syntax::Expression::Expression*> arguments =
        expression.get_right()->get_args();
    if (arguments.get_size() != 1) {
      return False;
    }

    ShaderValue coordinate;
    if (!emit_expression(arguments[0], &coordinate) ||
        coordinate.kind != ShaderValueKind::Vec2) {
      return False;
    }

    ShaderValue result;
    result.id = id_pool.take();
    result.kind = ShaderValueKind::Vec4;
    assembler.image_sample_implicit_lod(
        types.vec_4d_type_id, result.id, sampler.id, coordinate.id);
    *output = result;
    return True;
  }

  auto append_scalar_components(
      ShaderValue value,
      Dynamic::Vector<Bits_32>& components) -> Bool {
    ShaderValue source;
    if (!emit_load(value, &source)) {
      return False;
    }

    if (source.kind == ShaderValueKind::Real32) {
      components.insert(source.id);
      return True;
    }

    Count component_count = value_component_count(source.kind);
    if (component_count < 2 || component_count > 4) {
      return False;
    }

    for (Count i = 0; i < component_count; i++) {
      Bits_32 component_id = id_pool.take();
      Static::Vector<Bits_32, 1> indexes{Bits_32(i)};
      assembler.composite_extract(
          types.real_32_type_id, component_id, source.id, indexes);
      components.insert(component_id);
    }

    return True;
  }

  auto emit_pack(
      const Syntax::Expression::Expression& expression,
      ShaderValue* output) -> Bool {
    View::Vector<Syntax::Expression::Expression*> arguments = expression.get_args();
    View::Vector<Syntax::Pack::Field> named_fields;
    if (expression.get_kind() == Syntax::Expression::Kind::Pack) {
      const auto* pack = static_cast<const Syntax::Pack*>(&expression);
      named_fields = pack->get_fields();
    }

    if (arguments.get_size() == 0 && named_fields.get_size() == 0) {
      return False;
    }

    Dynamic::Vector<Bits_32> scalar_components;
    if (arguments.get_size() != 0) {
      for (Count i = 0; i < arguments.get_size(); i++) {
        ShaderValue argument;
        if (!emit_expression(arguments[i], &argument) ||
            !append_scalar_components(argument, scalar_components)) {
          return False;
        }
      }
    } else {
      for (Count field_index = 0; field_index < named_fields.get_size();
           field_index++) {
        ShaderValue field_value;
        if (!emit_expression(named_fields[field_index].value, &field_value) ||
            !append_scalar_components(field_value, scalar_components)) {
          Diagnostics::Log::Message<512> error(Diagnostics::Log::Level::Error);
          error << "Shader emitter failed to lower named pack field "_view
                << named_fields[field_index].name;
          return False;
        }
      }
    }

    ShaderValueKind result_kind = vector_kind(scalar_components.get_size());
    if (result_kind == ShaderValueKind::Invalid) {
      return False;
    }

    if (result_kind == ShaderValueKind::Real32) {
      *output = {scalar_components[0], ShaderValueKind::Real32, False};
      return True;
    }

    ShaderValue result;
    result.id = id_pool.take();
    result.kind = result_kind;
    assembler.composite_construct(
        type_id_for(types, result_kind), result.id,
        scalar_components.get_view());
    *output = result;
    return True;
  }

  auto emit_expression(
      const Syntax::Expression::Expression* expression,
      ShaderValue* output) -> Bool {
    if (!expression || !output) {
      return False;
    }

    switch (expression->get_kind()) {
    case Syntax::Expression::Kind::Literal:
    case Syntax::Expression::Kind::Unary:
      return emit_float_constant(*expression, output);

    case Syntax::Expression::Kind::Addressable:
    case Syntax::Expression::Kind::TypeAccess:
      return emit_addressable(*expression, output);

    case Syntax::Expression::Kind::FieldAccess:
      return emit_field_access(*expression, output);

    case Syntax::Expression::Kind::Index:
      return emit_index(*expression, output);

    case Syntax::Expression::Kind::Swizzle:
      return emit_swizzle(*expression, output);

    case Syntax::Expression::Kind::Binary:
      return emit_binary(*expression, output);

    case Syntax::Expression::Kind::Call:
      return emit_call(*expression, output);

    case Syntax::Expression::Kind::Pack:
      return emit_pack(*expression, output);

    case Syntax::Expression::Kind::Expression:
    case Syntax::Expression::Kind::Data:
    case Syntax::Expression::Kind::IndexSlice:
    case Syntax::Expression::Kind::Slice:
      return False;
    }

    return False;
  }

  auto emit_store(ShaderValue target, ShaderValue source) -> Bool {
    if (!target.is_pointer || target.kind != source.kind) {
      return False;
    }

    assembler.store(target.id, source.id);
    return True;
  }

  auto emit_named_return(
      const Syntax::Pack& pack) -> Bool {
    View::Vector<Syntax::Pack::Field> fields = pack.get_fields();
    for (Count i = 0; i < fields.get_size(); i++) {
      ShaderValue target;
      ShaderValue source;
      if (fields[i].name.is_empty() ||
          !find_output_field(fields[i].name, &target) ||
          !emit_expression(fields[i].value, &source) ||
          !emit_store(target, source)) {
        return False;
      }
    }

    return True;
  }

  auto emit_return(
      const Syntax::Expression::Expression* expression) -> Bool {
    if (!expression) {
      return False;
    }

    if (expression->get_kind() == Syntax::Expression::Kind::Pack) {
      const auto* pack = static_cast<const Syntax::Pack*>(expression);
      if (Syntax::Pack::has_named_field(pack->get_fields())) {
        return emit_named_return(*pack);
      }
    }

    ShaderValue target;
    ShaderValue source;
    return find_default_output(&target) && emit_expression(expression, &source) &&
           emit_store(target, source);
  }

  spirv& assembler;
  IdPool& id_pool;
  const ShaderTypes& types;
  View::Vector<FloatConstant> float_constants;
  View::Vector<NamedShaderValue> self_fields;
  View::Vector<NamedShaderValue> uniform_fields;
  View::Vector<NamedShaderValue> output_fields;
  Dynamic::Vector<NamedShaderValue> symbols;
};

static auto execution_model(
    Dialect::Shader::Stage stage,
    spirv::ExecutionModel* output) -> Bool {
  if (!output) {
    return False;
  }

  switch (stage) {
  case Dialect::Shader::Stage::Vertex:
    *output = spirv::ExecutionModel::Vertex;
    return True;
  case Dialect::Shader::Stage::Pixel:
    *output = spirv::ExecutionModel::Fragment;
    return True;
  case Dialect::Shader::Stage::None:
    return False;
  }

  return False;
}

static auto builtin_id(
    View::Bytes name,
    spirv::BuiltIn* output) -> Bool {
  if (!output) {
    return False;
  }

  if (name == "vertex_index"_view) {
    *output = spirv::BuiltIn::VertexIndex;
    return True;
  }
  if (name == "position"_view) {
    *output = spirv::BuiltIn::Position;
    return True;
  }

  return False;
}

static auto take_shader_types(IdPool& id_pool) -> ShaderTypes {
  ShaderTypes types;
  types.void_type_id = id_pool.take();
  types.function_type_id = id_pool.take();
  types.real_32_type_id = id_pool.take();
  types.uint_32_type_id = id_pool.take();
  types.signed_32_type_id = id_pool.take();
  types.vec_2d_type_id = id_pool.take();
  types.vec_3d_type_id = id_pool.take();
  types.vec_4d_type_id = id_pool.take();
  types.vec2_array_type_id = id_pool.take();
  types.vec2_array_pointer_type_id = id_pool.take();
  types.vec2_function_pointer_type_id = id_pool.take();
  types.image_type_id = id_pool.take();
  types.sampled_image_type_id = id_pool.take();
  types.sampled_image_pointer_type_id = id_pool.take();
  types.vec2_input_pointer_type_id = id_pool.take();
  types.vec2_output_pointer_type_id = id_pool.take();
  types.vec4_output_pointer_type_id = id_pool.take();
  types.int_input_pointer_type_id = id_pool.take();
  types.push_type_id = id_pool.take();
  types.push_pointer_type_id = id_pool.take();
  types.push_float_pointer_type_id = id_pool.take();
  types.push_vec2_pointer_type_id = id_pool.take();
  types.push_vec4_pointer_type_id = id_pool.take();
  return types;
}

static auto add_stage_variable(
    Dynamic::Vector<StageVariable>& variables,
    Dynamic::Vector<Bits_32>& interface_ids,
    IdPool& id_pool,
    View::Bytes name,
    ShaderValueKind kind,
    spirv::StorageClass storage,
    Dialect::Shader::Builtin builtin = Dialect::Shader::Builtin(),
    Count location = 0) -> ShaderValue {
  ShaderValue value;
  value.id = id_pool.take();
  value.kind = kind;
  value.is_pointer = True;
  value.storage = storage;
  variables.insert({name, value, builtin, location});
  interface_ids.insert(value.id);
  return value;
}

static auto add_descriptor_variable(
    Dynamic::Vector<StageVariable>& variables,
    IdPool& id_pool,
    const Dialect::Shader::DescriptorBinding& descriptor,
    ShaderValue* output) -> Bool {
  if (!output || !descriptor.type) {
    return False;
  }

  ShaderValueKind kind = value_kind_from_type_ref(*descriptor.type);
  if (kind != ShaderValueKind::SampledImage) {
    return False;
  }

  ShaderValue value;
  value.id = id_pool.take();
  value.kind = kind;
  value.is_pointer = True;
  value.storage = spirv::StorageClass::UniformConstant;

  StageVariable variable;
  variable.name = descriptor.name;
  variable.value = value;
  variable.descriptor_set = descriptor.set;
  variable.descriptor_slot = descriptor.slot;
  variable.descriptor = True;
  variables.insert(variable);
  *output = value;
  return True;
}

static auto find_push_field_value(
    View::Vector<PushFieldValue> fields,
    View::Bytes name,
    PushFieldValue* output) -> Bool {
  for (Count i = 0; i < fields.get_size(); i++) {
    if (fields[i].name == name) {
      if (output) {
        *output = fields[i];
      }
      return True;
    }
  }

  return False;
}

static auto emit_shader_module(
    Resolution::Resolver& resolver,
    const Resolution::Record& record,
    const Dialect::Shader::EntryPoint& entry,
    Dynamic::Bytes& output) -> Bool {
  if (!entry.type || !entry.func || entry.stage == Dialect::Shader::Stage::None) {
    return False;
  }

  Dialect::Shader::Metadata shader;
  Dialect::Shader::Interface interface =
      shader.interface_of(resolver, record, entry);
  Dynamic::Vector<Dialect::Shader::Vec2ArrayLiteral> source_arrays =
      shader.vec2_arrays(resolver, record);
  Dynamic::Vector<Dialect::Shader::DescriptorBinding> descriptors =
      shader.descriptors(resolver, record);
  PushInfo push = load_push_info(resolver, record);

  IdPool id_pool;
  const Bits_32 main_id = id_pool.take();
  const Bits_32 label_id = id_pool.take();
  ShaderTypes types = take_shader_types(id_pool);
  const Bits_32 array_length_id = id_pool.take();

  Dynamic::Vector<FloatConstant> float_constants;
  collect_body_float_constants(entry.func->get_body(), float_constants, id_pool);

  Dynamic::Vector<Vec2ArrayValue> arrays;
  for (Count i = 0; i < source_arrays.get_size(); i++) {
    if (source_arrays[i].value_count != quad_vertex_count) {
      return False;
    }

    Vec2ArrayValue array;
    array.name = source_arrays[i].name;
    array.values = source_arrays[i].values;
    array.value_count = source_arrays[i].value_count;
    for (Count vertex_index = 0; vertex_index < quad_vertex_count;
         vertex_index++) {
      array.x_component_ids[vertex_index] = id_pool.take();
      array.y_component_ids[vertex_index] = id_pool.take();
      array.vector_ids[vertex_index] = id_pool.take();
    }
    array.constant_id = id_pool.take();
    array.variable_id = id_pool.take();
    arrays.insert(array);
  }

  Dynamic::Vector<PushFieldValue> push_field_values;
  ShaderValue push_value;
  if (push.valid) {
    push_value.id = id_pool.take();
    push_value.is_pointer = True;
    push_value.storage = spirv::StorageClass::PushConstant;

    View::Vector<Dialect::Shader::HostField> fields = push_fields(push);
    for (Count i = 0; i < fields.get_size(); i++) {
      PushFieldValue field;
      field.name = fields[i].name;
      field.kind = value_kind_from_type_name(fields[i].type_name);
      field.index_id = id_pool.take();
      if (field.kind == ShaderValueKind::Invalid) {
        return False;
      }
      push_field_values.insert(field);
    }
  }

  Dynamic::Vector<Bits_32> interface_ids;
  Dynamic::Vector<StageVariable> variables;
  Dynamic::Vector<NamedShaderValue> self_fields;
  Dynamic::Vector<NamedShaderValue> uniform_fields;
  Dynamic::Vector<NamedShaderValue> input_symbols;
  Dynamic::Vector<NamedShaderValue> output_fields;
  Dynamic::Vector<View::Bytes> host_input_names;

  if (entry.stage == Dialect::Shader::Stage::Vertex) {
    View::Vector<Dialect::Shader::InterfaceField> inputs =
        interface.inputs.get_view();
    for (Count input_index = 0; input_index < inputs.get_size(); input_index++) {
      if (!inputs[input_index].builtin.present ||
          inputs[input_index].builtin.name != "vertex_index"_view) {
        continue;
      }

      Dialect::Shader::Builtin builtin;
      builtin.present = True;
      builtin.name = "vertex_index"_view;
      ShaderValue value = add_stage_variable(
          variables, interface_ids, id_pool, inputs[input_index].name,
          ShaderValueKind::Signed32, spirv::StorageClass::Input, builtin);
      input_symbols.insert({inputs[input_index].name, value});
    }
  } else {
    View::Vector<Dialect::Shader::InterfaceField> inputs =
        interface.inputs.get_view();
    for (Count i = 0; i < inputs.get_size(); i++) {
      ShaderValueKind kind = inputs[i].type
                                 ? value_kind_from_type_ref(*inputs[i].type)
                                 : ShaderValueKind::Invalid;
      if (!inputs[i].named || kind == ShaderValueKind::Invalid) {
        return False;
      }

      ShaderValue value = add_stage_variable(
          variables, interface_ids, id_pool, inputs[i].name, kind,
          spirv::StorageClass::Input, inputs[i].builtin, inputs[i].location);
      input_symbols.insert({inputs[i].name, value});
    }
  }

  View::Vector<Dialect::Shader::InterfaceField> outputs =
      interface.outputs.get_view();
  for (Count i = 0; i < outputs.get_size(); i++) {
    ShaderValueKind kind = outputs[i].type ? value_kind_from_type_ref(*outputs[i].type)
                                           : ShaderValueKind::Invalid;
    View::Bytes name = outputs[i].name;
    if (!outputs[i].named && kind == ShaderValueKind::Vec4) {
      name = "out_color"_view;
    }
    if (outputs[i].builtin.present && outputs[i].builtin.name == "position"_view) {
      kind = ShaderValueKind::Vec4;
    }
    if (kind == ShaderValueKind::Invalid) {
      return False;
    }

    ShaderValue value = add_stage_variable(
        variables, interface_ids, id_pool, name, kind, spirv::StorageClass::Output,
        outputs[i].builtin, outputs[i].location);
    output_fields.insert({name, value});
  }

  for (Count i = 0; i < descriptors.get_size(); i++) {
    ShaderValue value;
    if (!add_descriptor_variable(variables, id_pool, descriptors[i], &value)) {
      return False;
    }
    uniform_fields.insert({descriptors[i].name, value});
  }
  View::Vector<Dialect::Shader::HostField> host_fields =
      push_fields(push);
  for (Count host_input_index = 0; host_input_index < host_fields.get_size();
       host_input_index++) {
    host_input_names.insert(host_fields[host_input_index].name);
  }

  const Bool has_vec2_arrays = arrays.get_size() > 0;
  const Bool has_descriptors = descriptors.get_size() > 0;

  for (Count i = 0; i < arrays.get_size(); i++) {
    ShaderValue value;
    value.id = arrays[i].variable_id;
    value.kind = ShaderValueKind::Vec2Array;
    value.is_pointer = True;
    value.storage = spirv::StorageClass::Function;
    self_fields.insert({arrays[i].name, value});
    uniform_fields.insert({arrays[i].name, value});
  }

  spirv::ExecutionModel model = spirv::ExecutionModel::Vertex;
  if (!execution_model(entry.stage, &model)) {
    return False;
  }

  spirv assembler(output);
  assembler.begin_module(id_bound);
  assembler.capability(spirv::Capability::Shader);
  assembler.memory_model(
      spirv::AddressingModel::Logical, spirv::MemoryModel::GLSL450);

  assembler.entry_point(
      model, main_id, entry.func->get_definition().get_name(),
      interface_ids.get_view());
  if (entry.stage == Dialect::Shader::Stage::Pixel) {
    assembler.execution_mode(main_id, spirv::ExecutionMode::OriginUpperLeft);
  }

  assembler.name(main_id, entry.func->get_definition().get_name());
  for (Count i = 0; i < variables.get_size(); i++) {
    assembler.name(variables[i].value.id, variables[i].name);
  }
  for (Count i = 0; i < arrays.get_size(); i++) {
    assembler.name(arrays[i].variable_id, arrays[i].name);
  }
  if (push.valid) {
    emit_push_names(assembler, types.push_type_id, push_value.id, push);
  }

  for (Count i = 0; i < variables.get_size(); i++) {
    if (variables[i].descriptor) {
      assembler.decorate(
          variables[i].value.id, spirv::Decoration::DescriptorSet,
          Bits_32(variables[i].descriptor_set));
      assembler.decorate(
          variables[i].value.id, spirv::Decoration::Binding,
          Bits_32(variables[i].descriptor_slot));
      continue;
    }

    spirv::BuiltIn built_in = spirv::BuiltIn::Position;
    if (variables[i].builtin.present &&
        builtin_id(variables[i].builtin.name, &built_in)) {
      assembler.decorate(
          variables[i].value.id, spirv::Decoration::BuiltIn, Bits_32(built_in));
    } else {
      assembler.decorate(
          variables[i].value.id, spirv::Decoration::Location,
          Bits_32(variables[i].location));
    }
  }
  if (push.valid) {
    emit_push_decorations(assembler, types.push_type_id, push);
  }

  assembler.type_void(types.void_type_id);
  assembler.type_function(types.function_type_id, types.void_type_id);
  assembler.type_float(types.real_32_type_id, 32);
  assembler.type_int(types.uint_32_type_id, 32, False);
  assembler.type_int(types.signed_32_type_id, 32, True);
  assembler.type_vector(types.vec_2d_type_id, types.real_32_type_id, 2);
  assembler.type_vector(types.vec_3d_type_id, types.real_32_type_id, 3);
  assembler.type_vector(types.vec_4d_type_id, types.real_32_type_id, 4);
  if (has_vec2_arrays) {
    assembler.constant(
        types.uint_32_type_id, array_length_id, Bits_32(quad_vertex_count));
    assembler.type_array(
        types.vec2_array_type_id, types.vec_2d_type_id, array_length_id);
  }
  if (has_descriptors) {
    assembler.type_image(
        types.image_type_id, types.real_32_type_id, spirv::Dim::D2, 0, 0, 0,
        1, spirv::ImageFormat::Unknown);
    assembler.type_sampled_image(
        types.sampled_image_type_id, types.image_type_id);
    assembler.type_pointer(
        types.sampled_image_pointer_type_id,
        spirv::StorageClass::UniformConstant, types.sampled_image_type_id);
  }
  assembler.type_pointer(
      types.vec2_input_pointer_type_id, spirv::StorageClass::Input,
      types.vec_2d_type_id);
  assembler.type_pointer(
      types.vec2_output_pointer_type_id, spirv::StorageClass::Output,
      types.vec_2d_type_id);
  assembler.type_pointer(
      types.vec4_output_pointer_type_id, spirv::StorageClass::Output,
      types.vec_4d_type_id);
  assembler.type_pointer(
      types.int_input_pointer_type_id, spirv::StorageClass::Input,
      types.signed_32_type_id);
  if (has_vec2_arrays) {
    assembler.type_pointer(
        types.vec2_array_pointer_type_id, spirv::StorageClass::Function,
        types.vec2_array_type_id);
    assembler.type_pointer(
        types.vec2_function_pointer_type_id, spirv::StorageClass::Function,
        types.vec_2d_type_id);
  }
  if (push.valid) {
    emit_push_type(
        assembler, types.push_type_id, types.real_32_type_id,
        types.vec_2d_type_id, types.vec_4d_type_id, push);
    assembler.type_pointer(
        types.push_pointer_type_id, spirv::StorageClass::PushConstant,
        types.push_type_id);
    assembler.type_pointer(
        types.push_float_pointer_type_id, spirv::StorageClass::PushConstant,
        types.real_32_type_id);
    assembler.type_pointer(
        types.push_vec2_pointer_type_id, spirv::StorageClass::PushConstant,
        types.vec_2d_type_id);
    assembler.type_pointer(
        types.push_vec4_pointer_type_id, spirv::StorageClass::PushConstant,
        types.vec_4d_type_id);
  }

  for (Count i = 0; i < push_field_values.get_size(); i++) {
    assembler.constant(
        types.signed_32_type_id, push_field_values[i].index_id, Bits_32(i));
  }
  for (Count i = 0; i < float_constants.get_size(); i++) {
    assembler.constant(
        types.real_32_type_id, float_constants[i].id, float_constants[i].bits);
  }
  for (Count i = 0; i < arrays.get_size(); i++) {
    emit_vec2_constants(
        assembler, types.real_32_type_id, types.vec_2d_type_id,
        arrays[i].values, arrays[i].x_component_ids, arrays[i].y_component_ids,
        arrays[i].vector_ids);
    assembler.constant_composite(
        types.vec2_array_type_id, arrays[i].constant_id, arrays[i].vector_ids);
  }

  for (Count i = 0; i < variables.get_size(); i++) {
    Bits_32 pointer_type_id = pointer_type_id_for(
        types, variables[i].value.kind, variables[i].value.storage);
    if (pointer_type_id == 0) {
      return False;
    }

    assembler.variable(
        pointer_type_id, variables[i].value.id, variables[i].value.storage);
  }
  if (push.valid) {
    assembler.variable(
        types.push_pointer_type_id, push_value.id,
        spirv::StorageClass::PushConstant);
  }

  for (Count host_input_index = 0;
       host_input_index < host_input_names.get_size();
       host_input_index++) {
    PushFieldValue host_input;
    if (!find_push_field_value(
            push_field_values.get_view(), host_input_names[host_input_index],
            &host_input)) {
      return False;
    }

    Bits_32 pointer_type_id = pointer_type_id_for(
        types, host_input.kind, spirv::StorageClass::PushConstant);
    if (pointer_type_id == 0) {
      return False;
    }

    ShaderValue value;
    value.id = id_pool.take();
    value.kind = host_input.kind;
    value.is_pointer = True;
    value.storage = spirv::StorageClass::PushConstant;
    Static::Vector<Bits_32, 1> indexes(host_input.index_id);
    assembler.access_chain(pointer_type_id, value.id, push_value.id, indexes);
    uniform_fields.insert({host_input.name, value});
  }

  assembler.function(
      types.void_type_id, main_id, spirv::FunctionControl::None,
      types.function_type_id);
  assembler.label(label_id);
  for (Count i = 0; i < arrays.get_size(); i++) {
    assembler.variable(
        types.vec2_array_pointer_type_id, arrays[i].variable_id,
        spirv::StorageClass::Function);
    assembler.store(arrays[i].variable_id, arrays[i].constant_id);
  }

  ShaderBodyEmitter body_emitter(
      assembler, id_pool, types, float_constants.get_view(),
      self_fields.get_view(), uniform_fields.get_view(),
      output_fields.get_view());
  for (Count i = 0; i < input_symbols.get_size(); i++) {
    body_emitter.add_symbol(input_symbols[i].name, input_symbols[i].value);
  }
  if (!body_emitter.emit_body(entry.func->get_body())) {
    return False;
  }

  assembler.return_void();
  assembler.function_end();

  return spirv::is_valid_module(output);
}

auto Tetrodotoxin::Compiler::Shader::emit_stage_module(
    Resolution::Resolver& resolver,
    const Resolution::Record& record,
    const Dialect::Shader::EntryPoint& entry,
    Dynamic::Bytes& output) -> Bool {
  output.clear();

  spirv::ExecutionModel model = spirv::ExecutionModel::Vertex;
  if (!execution_model(entry.stage, &model)) {
    return False;
  }

  switch (entry.stage) {
  case Dialect::Shader::Stage::Vertex:
  case Dialect::Shader::Stage::Pixel:
    return emit_shader_module(resolver, record, entry, output);
  case Dialect::Shader::Stage::None:
    return False;
  }

  return False;
}

auto Tetrodotoxin::Compiler::Shader::emit_program_modules(
    Resolution::Resolver& resolver,
    const Resolution::Record& record,
    Dynamic::Bytes& vertex,
    Dynamic::Bytes& pixel) -> Bool {
  vertex.clear();
  pixel.clear();

  Dialect::Shader::Metadata shader;
  Dynamic::Vector<Dialect::Shader::EntryPoint> entries =
      shader.entry_points(record);

  Bool emitted_vertex = False;
  Bool emitted_pixel = False;
  for (Count i = 0; i < entries.get_size(); i++) {
    switch (entries[i].stage) {
    case Dialect::Shader::Stage::Vertex:
      if (emitted_vertex) {
        Diagnostics::Log::Message<512> error(Diagnostics::Log::Level::Error);
        error << "Shader package emitted duplicate vertex stage: "_view
              << record.get_path();
        return False;
      }
      if (!emit_stage_module(resolver, record, entries[i], vertex)) {
        Diagnostics::Log::Message<512> error(Diagnostics::Log::Level::Error);
        error << "Failed to emit vertex shader stage for "_view
              << record.get_path();
        return False;
      }
      emitted_vertex = True;
      break;
    case Dialect::Shader::Stage::Pixel:
      if (emitted_pixel) {
        Diagnostics::Log::Message<512> error(Diagnostics::Log::Level::Error);
        error << "Shader package emitted duplicate pixel stage: "_view
              << record.get_path();
        return False;
      }
      if (!emit_stage_module(resolver, record, entries[i], pixel)) {
        Diagnostics::Log::Message<512> error(Diagnostics::Log::Level::Error);
        error << "Failed to emit pixel shader stage for "_view
              << record.get_path();
        return False;
      }
      emitted_pixel = True;
      break;
    case Dialect::Shader::Stage::None:
      break;
    }
  }

  if (!emitted_vertex || !emitted_pixel) {
    Diagnostics::Log::Message<512> error(Diagnostics::Log::Level::Error);
    error << "Shader package must emit both vertex and pixel stages: "_view
          << record.get_path();
  }

  return emitted_vertex && emitted_pixel;
}
