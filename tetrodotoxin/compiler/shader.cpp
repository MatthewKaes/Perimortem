// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/compiler/shader.hpp"

#include "perimortem/core/static/vector.hpp"

#include "perimortem/memory/managed/vector.hpp"

#include "perimortem/utility/table.hpp"

#include "tetrodotoxin/compiler/assembler/spir_v.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Perimortem::Utility;
using namespace Tetrodotoxin;
using namespace Tetrodotoxin::Compiler;

// These are local SPIR-V result ids.
//
// SPIR-V instructions refer to typed objects by numeric ids. The fixed entries
// below reserve ids for the shared types, pointer types, entry function, and
// other objects this small stage compiler emits for every module.
//
// The large numeric gaps are intentional range partitions. `parameter_id(i)`,
// `result_id(i)`, and `resource_id(i)` derive ids by adding `i` to a base.
// Leaving each family in a visible range keeps ids stable as shared helper
// types are added and makes disassembly/archive inspection less miserable than
// a dense "whatever came next" allocator.
enum class ResultId : Bits_32 {
  Invalid = 0,
  EntryFunction = 1,
  Void = 2,
  VoidFunction = 3,
  EntryLabel = 4,
  Bits32 = 5,
  Real32 = 6,
  Vec2 = 7,
  Vec4 = 8,
  Uvec2 = 9,
  Image2D = 10,
  SampledImage = 11,
  InputBits32 = 12,
  InputVec2 = 13,
  InputVec4 = 14,
  InputReal32 = 15,
  InputUvec2 = 16,
  OutputBits32 = 17,
  OutputVec2 = 18,
  OutputVec4 = 19,
  OutputReal32 = 20,
  OutputUvec2 = 21,
  PushStruct = 22,
  PushPointer = 23,
  ResourcePointer = 24,

  // Derived per-stage ids. These ranges can move behind a real id allocator
  // once body lowering starts producing arbitrary SSA temporaries, but fixed
  // bands are simpler and clearer for the current declaration-only modules.
  ParameterBase = 64,
  ResultBase = 96,
  PushVariable = 120,
  ResourceBase = 128,
};

static constexpr Bits_32 stage_id_bound = 256;

static constexpr Static::Vector<Pair<View::Bytes, ResultId>, 5>
    spirv_type_names = {{
      {"Bits_32"_view, ResultId::Bits32},
      {"Real_32"_view, ResultId::Real32},
      {"Vec2D"_view, ResultId::Vec2},
      {"Vec4D"_view, ResultId::Vec4},
      {"Size2D"_view, ResultId::Uvec2},
    }};

using SpirvTypeNames = Table<ResultId, spirv_type_names>;

struct ShapeRule {
  Count member_count;
  ResultId member_type;
  ResultId result_type;
};

static constexpr Static::Vector<ShapeRule, 3> spirv_shape_rules = {{
  {2, ResultId::Real32, ResultId::Vec2},
  {4, ResultId::Real32, ResultId::Vec4},
  {2, ResultId::Bits32, ResultId::Uvec2},
}};

struct PointerDecl {
  ResultId pointer;
  ResultId pointee;
  Assembler::SpirV::StorageClass storage;
};

// Pointer declarations and lookup share one table so adding a supported SPIR-V
// value type does not create two maintenance points.
static constexpr Static::Vector<PointerDecl, 11> core_pointer_decls = {{
  {ResultId::InputBits32, ResultId::Bits32,
   Assembler::SpirV::StorageClass::Input},
  {ResultId::InputVec2, ResultId::Vec2, Assembler::SpirV::StorageClass::Input},
  {ResultId::InputVec4, ResultId::Vec4, Assembler::SpirV::StorageClass::Input},
  {ResultId::InputReal32, ResultId::Real32,
   Assembler::SpirV::StorageClass::Input},
  {ResultId::InputUvec2, ResultId::Uvec2,
   Assembler::SpirV::StorageClass::Input},
  {ResultId::OutputBits32, ResultId::Bits32,
   Assembler::SpirV::StorageClass::Output},
  {ResultId::OutputVec2, ResultId::Vec2,
   Assembler::SpirV::StorageClass::Output},
  {ResultId::OutputVec4, ResultId::Vec4,
   Assembler::SpirV::StorageClass::Output},
  {ResultId::OutputReal32, ResultId::Real32,
   Assembler::SpirV::StorageClass::Output},
  {ResultId::OutputUvec2, ResultId::Uvec2,
   Assembler::SpirV::StorageClass::Output},
  {ResultId::ResourcePointer, ResultId::SampledImage,
   Assembler::SpirV::StorageClass::UniformConstant},
}};

static constexpr auto spirv_id(ResultId value) -> Bits_32 {
  return Bits_32(value);
}

static auto type_has_name(const Ttx::Type* type, View::Bytes name) -> Bool {
  if (type == nullptr) {
    return False;
  }

  // Authored names let domain types such as Image keep their source identity,
  // while canonical names let aliases such as Point2D behave like their target
  // Vec2D during backend lowering.
  if (type->get_name() == name) {
    return True;
  }

  const Ttx::Type* canonical = type->canonical();
  return canonical != nullptr && canonical->get_name() == name;
}

static auto named_spirv_type_id(const Ttx::Type* type) -> ResultId {
  if (type == nullptr) {
    return ResultId::Invalid;
  }

  ResultId result =
      SpirvTypeNames::find_or_default(type->get_name(), ResultId::Invalid);
  if (result != ResultId::Invalid) {
    return result;
  }

  const Ttx::Type* canonical = type->canonical();
  return canonical == nullptr || canonical == type
             ? ResultId::Invalid
             : SpirvTypeNames::find_or_default(
                   canonical->get_name(), ResultId::Invalid);
}

static auto member_shape_matches(
    View::Vector<Ttx::Type::Member> members,
    Count member_count,
    ResultId member_type_id) -> Bool {
  // This is the narrow backend equivalent of asking whether a layout projects
  // to N identical primitive storage slots. Once Layout exposes that query
  // directly this should collapse into the shared TTX Layout API.
  if (members.get_size() != member_count) {
    return False;
  }

  for (Count i = 0; i < members.get_size(); i++) {
    if (named_spirv_type_id(members[i].get_type()) != member_type_id) {
      return False;
    }
  }

  return True;
}

// This is deliberately shape based. Render and Library own the source facts;
// Shader lowering only recognizes the small value surface it can turn into
// SPIR-V today, including aliases such as Point2D and Color.
static auto spirv_type_id(const Ttx::Type* type) -> ResultId {
  ResultId result = named_spirv_type_id(type);
  if (result != ResultId::Invalid) {
    return result;
  }

  const Ttx::Type* canonical = type == nullptr ? nullptr : type->canonical();
  if (canonical == nullptr) {
    return ResultId::Invalid;
  }

  View::Vector<Ttx::Type::Member> members = canonical->get_members();
  for (Count i = 0; i < spirv_shape_rules.get_size(); i++) {
    if (member_shape_matches(
            members, spirv_shape_rules[i].member_count,
            spirv_shape_rules[i].member_type)) {
      return spirv_shape_rules[i].result_type;
    }
  }

  return ResultId::Invalid;
}

static auto pointer_type_id(
    ResultId type_id,
    Assembler::SpirV::StorageClass storage) -> ResultId {
  for (Count i = 0; i < core_pointer_decls.get_size(); i++) {
    if (core_pointer_decls[i].storage == storage &&
        core_pointer_decls[i].pointee == type_id) {
      return core_pointer_decls[i].pointer;
    }
  }

  return ResultId::Invalid;
}

static auto spirv_type_size(ResultId type_id) -> Bits_32 {
  switch (type_id) {
  case ResultId::Bits32:
  case ResultId::Real32:
    return 4;

  case ResultId::Vec2:
  case ResultId::Uvec2:
    return 8;

  case ResultId::Vec4:
    return 16;

  default:
    return 0;
  }
}

static auto parameter_id(Count index) -> ResultId {
  return ResultId(spirv_id(ResultId::ParameterBase) + Bits_32(index));
}

static auto result_id(Count index) -> ResultId {
  return ResultId(spirv_id(ResultId::ResultBase) + Bits_32(index));
}

static auto resource_id(Count index) -> ResultId {
  return ResultId(spirv_id(ResultId::ResourceBase) + Bits_32(index));
}

static auto is_texture_resource(const Ttx::Type* type) -> Bool {
  return type != nullptr && (type_has_name(type, "Image"_view) ||
                             type->find_function("sample"_view) != nullptr);
}

static auto emit_core_types(Assembler::SpirV& assembler) -> void {
  // Core type declarations are the SPIR-V type vocabulary this compiler knows
  // how to produce today. Higher-level TTX names are lowered to these ids
  // before variables or instructions are emitted.
  assembler.type_void(spirv_id(ResultId::Void));
  assembler.type_function(
      spirv_id(ResultId::VoidFunction), spirv_id(ResultId::Void));
  assembler.type_int(spirv_id(ResultId::Bits32), 32, False);
  assembler.type_float(spirv_id(ResultId::Real32), 32);
  assembler.type_vector(
      spirv_id(ResultId::Vec2), spirv_id(ResultId::Real32), 2);
  assembler.type_vector(
      spirv_id(ResultId::Vec4), spirv_id(ResultId::Real32), 4);
  assembler.type_vector(
      spirv_id(ResultId::Uvec2), spirv_id(ResultId::Bits32), 2);
  assembler.type_image(
      spirv_id(ResultId::Image2D), spirv_id(ResultId::Real32),
      Assembler::SpirV::Dim::D2, 0, 0, 0, 1,
      Assembler::SpirV::ImageFormat::Unknown);
  assembler.type_sampled_image(
      spirv_id(ResultId::SampledImage), spirv_id(ResultId::Image2D));

  for (Count i = 0; i < core_pointer_decls.get_size(); i++) {
    assembler.type_pointer(
        spirv_id(core_pointer_decls[i].pointer), core_pointer_decls[i].storage,
        spirv_id(core_pointer_decls[i].pointee));
  }
}

static auto begin_stage_module(
    Assembler::SpirV& assembler,
    Assembler::SpirV::ExecutionModel model,
    View::Vector<Bits_32> interface_ids) -> void {
  // A shader module starts with the normal SPIR-V header and declares the entry
  // point before any debug names, decorations, types, variables, or functions.
  // The bound is one greater than every id the fixed stage ranges may emit.
  assembler.begin_module(stage_id_bound);
  assembler.capability(Assembler::SpirV::Capability::Shader);
  assembler.memory_model(
      Assembler::SpirV::AddressingModel::Logical,
      Assembler::SpirV::MemoryModel::GLSL450);
  assembler.entry_point(
      model, spirv_id(ResultId::EntryFunction), "main"_view, interface_ids);
  if (model == Assembler::SpirV::ExecutionModel::Fragment) {
    assembler.execution_mode(
        spirv_id(ResultId::EntryFunction),
        Assembler::SpirV::ExecutionMode::OriginUpperLeft);
  }
}

static auto emit_names(
    Assembler::SpirV& assembler,
    View::Vector<Ttx::Type::Member> parameters,
    View::Vector<Ttx::Type::Member> results,
    View::Vector<Ttx::Type::Member> push_members,
    View::Vector<Ttx::Type::Member> resource_members) -> void {
  // OpName/OpMemberName are debug instructions, not declarations. SPIR-V allows
  // them to reference ids before those ids are defined, and the logical module
  // layout places debug names before annotations, type declarations, variables,
  // and function bodies.
  for (Count i = 0; i < parameters.get_size(); i++) {
    assembler.name(spirv_id(parameter_id(i)), parameters[i].get_name());
  }

  for (Count i = 0; i < results.get_size(); i++) {
    assembler.name(spirv_id(result_id(i)), results[i].get_name());
  }

  if (!push_members.is_empty()) {
    assembler.name(spirv_id(ResultId::PushVariable), "push"_view);
    for (Count i = 0; i < push_members.get_size(); i++) {
      assembler.member_name(
          spirv_id(ResultId::PushStruct), Bits_32(i),
          push_members[i].get_name());
    }
  }

  for (Count i = 0; i < resource_members.get_size(); i++) {
    assembler.name(spirv_id(resource_id(i)), resource_members[i].get_name());
  }
}

static auto emit_interface_decorations(
    Assembler::SpirV& assembler,
    Assembler::SpirV::ExecutionModel model,
    View::Vector<Ttx::Type::Member> parameters,
    View::Vector<Ttx::Type::Member> results) -> void {
  // Render stage signatures become the shader entry interface. The current
  // naming convention maps vertex_index and screen_position to Vulkan builtins;
  // the remaining stage handoff values use dense locations.
  Count input_location = 0;
  for (Count i = 0; i < parameters.get_size(); i++) {
    if (model == Assembler::SpirV::ExecutionModel::Vertex &&
        parameters[i].get_name() == "vertex_index"_view) {
      assembler.decorate(
          spirv_id(parameter_id(i)), Assembler::SpirV::Decoration::BuiltIn,
          Bits_32(Assembler::SpirV::BuiltIn::VertexIndex));
      continue;
    }

    assembler.decorate(
        spirv_id(parameter_id(i)), Assembler::SpirV::Decoration::Location,
        Bits_32(input_location++));
  }

  Count output_location = 0;
  for (Count i = 0; i < results.get_size(); i++) {
    if (model == Assembler::SpirV::ExecutionModel::Vertex &&
        results[i].get_name() == "screen_position"_view) {
      assembler.decorate(
          spirv_id(result_id(i)), Assembler::SpirV::Decoration::BuiltIn,
          Bits_32(Assembler::SpirV::BuiltIn::Position));
      continue;
    }

    assembler.decorate(
        spirv_id(result_id(i)), Assembler::SpirV::Decoration::Location,
        Bits_32(output_location++));
  }
}

static auto emit_stage_storage_decorations(
    Assembler::SpirV& assembler,
    View::Vector<ResultId> push_member_type_ids,
    View::Vector<Ttx::Type::Member> resource_members) -> void {
  // Push and resource facts are owned by Render. Shader lowering only emits the
  // storage surfaces the stage declared it can read.
  if (!push_member_type_ids.is_empty()) {
    assembler.decorate(
        spirv_id(ResultId::PushStruct), Assembler::SpirV::Decoration::Block);
    Bits_32 offset = 0;
    for (Count i = 0; i < push_member_type_ids.get_size(); i++) {
      assembler.member_decorate(
          spirv_id(ResultId::PushStruct), Bits_32(i),
          Assembler::SpirV::Decoration::Offset, offset);
      offset += spirv_type_size(push_member_type_ids[i]);
    }
  }

  for (Count i = 0; i < resource_members.get_size(); i++) {
    assembler.decorate(
        spirv_id(resource_id(i)), Assembler::SpirV::Decoration::DescriptorSet,
        0);
    assembler.decorate(
        spirv_id(resource_id(i)), Assembler::SpirV::Decoration::Binding,
        Bits_32(i));
  }
}

static auto emit_push_type(
    Allocator::Arena& arena,
    Assembler::SpirV& assembler,
    View::Vector<ResultId> push_member_type_ids) -> void {
  // Push constants are represented as a Block-decorated struct plus a pointer
  // to that struct in the PushConstant storage class. The offsets were emitted
  // in the annotation phase; this phase declares the actual structural type.
  if (push_member_type_ids.is_empty()) {
    return;
  }

  Managed::Vector<Bits_32> push_member_type_words(arena);
  for (Count i = 0; i < push_member_type_ids.get_size(); i++) {
    push_member_type_words.insert(spirv_id(push_member_type_ids[i]));
  }

  assembler.type_struct(
      spirv_id(ResultId::PushStruct), push_member_type_words.get_view());
  assembler.type_pointer(
      spirv_id(ResultId::PushPointer),
      Assembler::SpirV::StorageClass::PushConstant,
      spirv_id(ResultId::PushStruct));
}

static auto emit_variables(
    Assembler::SpirV& assembler,
    View::Vector<Ttx::Type::Member> parameters,
    View::Vector<Ttx::Type::Member> results,
    Bool has_push,
    View::Vector<Ttx::Type::Member> resource_members) -> void {
  // Variables bind the interface ids from OpEntryPoint to concrete pointer
  // types and storage classes. Shader inputs, outputs, push constants, and
  // resources are all global variables from SPIR-V's point of view.
  for (Count i = 0; i < parameters.get_size(); i++) {
    ResultId type_id = spirv_type_id(parameters[i].get_type());
    assembler.variable(
        spirv_id(
            pointer_type_id(type_id, Assembler::SpirV::StorageClass::Input)),
        spirv_id(parameter_id(i)), Assembler::SpirV::StorageClass::Input);
  }

  for (Count i = 0; i < results.get_size(); i++) {
    ResultId type_id = spirv_type_id(results[i].get_type());
    assembler.variable(
        spirv_id(
            pointer_type_id(type_id, Assembler::SpirV::StorageClass::Output)),
        spirv_id(result_id(i)), Assembler::SpirV::StorageClass::Output);
  }

  if (has_push) {
    assembler.variable(
        spirv_id(ResultId::PushPointer), spirv_id(ResultId::PushVariable),
        Assembler::SpirV::StorageClass::PushConstant);
  }

  for (Count i = 0; i < resource_members.get_size(); i++) {
    assembler.variable(
        spirv_id(ResultId::ResourcePointer), spirv_id(resource_id(i)),
        Assembler::SpirV::StorageClass::UniformConstant);
  }
}

static auto emit_empty_entry_point(Assembler::SpirV& assembler) -> void {
  // Body lowering has not landed yet; the entry point is valid and exposes the
  // source-shaped interface, but it intentionally performs no work today.
  assembler.function(
      spirv_id(ResultId::Void), spirv_id(ResultId::EntryFunction),
      Assembler::SpirV::FunctionControl::None,
      spirv_id(ResultId::VoidFunction));
  assembler.label(spirv_id(ResultId::EntryLabel));
  assembler.return_void();
  assembler.function_end();
}

auto Shader::lower(Context& context, View::Bytes module, const Ttx::Type& root)
    -> Bool {
  if (root.attribute_equals("isa"_view, "RenderSource"_view) ||
      root.attribute_equals("isa"_view, "Render"_view)) {
    register_render_contracts(root);
    return True;
  }

  if (!root.attribute_equals("isa"_view, "Shader"_view)) {
    return True;
  }

  const Ttx::Type* contract = find_contract(root);
  if (contract == nullptr) {
    return context.report(
        "Shader compiler could not find render contract."_view);
  }

  View::Vector<Ttx::Type::Function> stages = root.get_functions();
  if (stages.is_empty()) {
    return context.report("Shader source did not declare any stages."_view);
  }

  for (Count i = 0; i < stages.get_size(); i++) {
    if (!lower_stage(context, module, root, *contract, stages[i])) {
      return False;
    }
  }

  return True;
}

auto Shader::lower_stage(
    Context& context,
    View::Bytes module,
    const Ttx::Type& shader,
    const Ttx::Type& contract,
    const Ttx::Type::Function& stage) -> Bool {
  Assembler::SpirV::ExecutionModel model;
  if (stage.get_name() == "vertex"_view) {
    model = Assembler::SpirV::ExecutionModel::Vertex;
  } else if (stage.get_name() == "pixel"_view) {
    model = Assembler::SpirV::ExecutionModel::Fragment;
  } else {
    return context.report(
        "Only vertex and pixel shader stages can lower today."_view);
  }

  // Render owns the pipeline contract. Shader lowering consumes the same stage
  // facts the ISA validator already checked instead of creating a compiler-side
  // mirror for push constants and resources.
  const Ttx::Type* stage_facts = contract.find_type(stage.get_name());
  if (stage_facts == nullptr) {
    return context.report("Shader stage has no render fact contract."_view);
  }

  const Ttx::Type* push = stage_facts->find_type("push"_view);
  const Ttx::Type* resource = stage_facts->find_type("resource"_view);
  View::Vector<Ttx::Type::Member> parameters = stage.get_parameters();
  View::Vector<Ttx::Type::Member> results = stage.get_result();
  View::Vector<Ttx::Type::Member> push_members =
      push == nullptr ? View::Vector<Ttx::Type::Member>() : push->get_members();
  View::Vector<Ttx::Type::Member> resource_members =
      resource == nullptr ? View::Vector<Ttx::Type::Member>()
                          : resource->get_members();

  Managed::Vector<ResultId> push_member_type_ids(context.get_arena());
  for (Count i = 0; i < push_members.get_size(); i++) {
    ResultId type_id = spirv_type_id(push_members[i].get_type());
    if (type_id == ResultId::Invalid) {
      return context.report(
          "Shader push constant type cannot lower today."_view);
    }
    push_member_type_ids.insert(type_id);
  }

  for (Count i = 0; i < resource_members.get_size(); i++) {
    if (!is_texture_resource(resource_members[i].get_type())) {
      return context.report("Shader resource type cannot lower today."_view);
    }
  }

  Managed::Vector<Bits_32> interface_ids(context.get_arena());
  for (Count i = 0; i < parameters.get_size(); i++) {
    ResultId type_id = spirv_type_id(parameters[i].get_type());
    if (type_id == ResultId::Invalid) {
      return context.report("Shader parameter type cannot lower today."_view);
    }

    interface_ids.insert(spirv_id(parameter_id(i)));
  }

  for (Count i = 0; i < results.get_size(); i++) {
    ResultId type_id = spirv_type_id(results[i].get_type());
    if (type_id == ResultId::Invalid) {
      return context.report("Shader result type cannot lower today."_view);
    }

    interface_ids.insert(spirv_id(result_id(i)));
  }

  Dynamic::Bytes words;
  Assembler::SpirV assembler(words);

  // Keep the emission order close to SPIR-V's logical layout:
  // header/import facts, debug names, annotations, type/global declarations,
  // then function bodies. The helpers above are split along those section
  // lines.
  begin_stage_module(assembler, model, interface_ids.get_view());
  emit_names(assembler, parameters, results, push_members, resource_members);
  emit_interface_decorations(assembler, model, parameters, results);
  emit_stage_storage_decorations(
      assembler, push_member_type_ids.get_view(), resource_members);
  emit_core_types(assembler);
  emit_push_type(
      context.get_arena(), assembler, push_member_type_ids.get_view());
  emit_variables(
      assembler, parameters, results, !push_member_type_ids.is_empty(),
      resource_members);
  emit_empty_entry_point(assembler);

  if (!Assembler::SpirV::is_valid_module(words)) {
    return context.report("Shader compiler emitted invalid SPIR-V."_view);
  }

  const Count offset = read_only.get_size();
  read_only.concat(words.get_view());
  stages.insert({
    stage_name(context, module, shader.get_name(), stage.get_name()),
    {offset, words.get_size()},
  });
  return True;
}

auto Shader::register_render_contracts(const Ttx::Type& root) -> void {
  if (root.attribute_equals("isa"_view, "Render"_view)) {
    register_render_contract(root);
    return;
  }

  View::Vector<const Ttx::Type*> types = root.get_types();
  for (Count i = 0; i < types.get_size(); i++) {
    if (types[i] != nullptr &&
        types[i]->attribute_equals("isa"_view, "Render"_view)) {
      register_render_contract(*types[i]);
    }
  }
}

auto Shader::register_render_contract(const Ttx::Type& render) -> void {
  contracts.insert(render.get_name(), &render);
}

auto Shader::find_contract(const Ttx::Type& shader) const -> const Ttx::Type* {
  const Ttx::Attribute* contract =
      shader.resolve_attribute("contract"_view);
  if (contract == nullptr) {
    return nullptr;
  }

  const auto* entry = contracts.find(contract->get_value());
  return entry == nullptr ? nullptr : entry->value;
}

auto Shader::stage_name(
    Context& context,
    View::Bytes module,
    View::Bytes shader,
    View::Bytes stage) -> View::Bytes {
  Managed::Bytes output(context.get_arena());
  output.concat("TTX_shader_"_view);
  append_name_segment(output, module);
  output.append('_');
  output.concat(shader);
  output.append('_');
  output.concat(stage);
  output.concat("_spirv"_view);
  return output;
}

auto Shader::append_name_segment(Managed::Bytes& output, View::Bytes value)
    -> void {
  // Module names are derived from source filenames, not TTX identifiers, so the
  // exported stage name needs a normalized module segment.
  for (Count i = 0; i < value.get_size(); i++) {
    Bits_8 c = value[i];
    const Bool alpha = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
    const Bool digit = c >= '0' && c <= '9';
    output.append(alpha || digit || c == '_' ? c : Bits_8('_'));
  }
}
