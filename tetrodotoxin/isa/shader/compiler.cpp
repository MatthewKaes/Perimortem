// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/isa/shader/compiler.hpp"

#include "perimortem/core/static/vector.hpp"

#include "perimortem/memory/managed/vector.hpp"

#include "perimortem/utility/table.hpp"

#include "tetrodotoxin/compiler/assembler/spir_v.hpp"
#include "tetrodotoxin/isa/shader/block.hpp"
#include "tetrodotoxin/standard/types.hpp"
#include "ttx/layout.hpp"

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
enum class SpirvId : Unsigned_32 {
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
  TemporaryBase = 160,
};

static constexpr Unsigned_32 stage_id_bound = 256;
static constexpr View::Bytes shader_type_attribute = "shader_type"_view;

static constexpr Static::Vector<Pair<View::Bytes, SpirvId>, 4>
    spirv_type_names = {{
      {"Unsigned_32"_view, SpirvId::Bits32},
      {"Real_32"_view, SpirvId::Real32},
      {"Vec2D"_view, SpirvId::Vec2},
      {"Vec4D"_view, SpirvId::Vec4},
    }};

using SpirvTypeNames = Table<SpirvId, spirv_type_names>;

static constexpr Static::Vector<Pair<View::Bytes, SpirvId>, 5>
    shader_type_names = {{
      {"Unsigned_32"_view, SpirvId::Bits32},
      {"Real_32"_view, SpirvId::Real32},
      {"Vec2D"_view, SpirvId::Vec2},
      {"Vec4D"_view, SpirvId::Vec4},
      {"Uvec2"_view, SpirvId::Uvec2},
    }};

using ShaderTypeNames = Table<SpirvId, shader_type_names>;

static constexpr Static::
    Vector<Pair<View::Bytes, Assembler::SpirV::ExecutionModel>, 2>
        execution_models = {{
          {"vertex"_view, Assembler::SpirV::ExecutionModel::Vertex},
          {"pixel"_view, Assembler::SpirV::ExecutionModel::Fragment},
        }};

using ExecutionModels =
    Table<Assembler::SpirV::ExecutionModel, execution_models>;

struct PointerDecl {
  SpirvId pointer;
  SpirvId pointee;
  Assembler::SpirV::StorageClass storage;
};

// Pointer declarations and lookup share one table so adding a supported SPIR-V
// value type does not create two maintenance points.
static constexpr Static::Vector<PointerDecl, 11> core_pointer_decls = {{
  {SpirvId::InputBits32, SpirvId::Bits32,
   Assembler::SpirV::StorageClass::Input},
  {SpirvId::InputVec2, SpirvId::Vec2, Assembler::SpirV::StorageClass::Input},
  {SpirvId::InputVec4, SpirvId::Vec4, Assembler::SpirV::StorageClass::Input},
  {SpirvId::InputReal32, SpirvId::Real32,
   Assembler::SpirV::StorageClass::Input},
  {SpirvId::InputUvec2, SpirvId::Uvec2, Assembler::SpirV::StorageClass::Input},
  {SpirvId::OutputBits32, SpirvId::Bits32,
   Assembler::SpirV::StorageClass::Output},
  {SpirvId::OutputVec2, SpirvId::Vec2, Assembler::SpirV::StorageClass::Output},
  {SpirvId::OutputVec4, SpirvId::Vec4, Assembler::SpirV::StorageClass::Output},
  {SpirvId::OutputReal32, SpirvId::Real32,
   Assembler::SpirV::StorageClass::Output},
  {SpirvId::OutputUvec2, SpirvId::Uvec2,
   Assembler::SpirV::StorageClass::Output},
  {SpirvId::ResourcePointer, SpirvId::SampledImage,
   Assembler::SpirV::StorageClass::UniformConstant},
}};

static constexpr auto spirv_id(SpirvId value) -> Unsigned_32 {
  return Unsigned_32(value);
}

static auto type_has_name(const Ttx::Type& type, View::Bytes name) -> Bool {
  // Authored names let domain types such as Image keep their source identity,
  // while canonical names let aliases such as Point2D behave like their target
  // Vec2D during backend lowering.
  if (type.get_name() == name) {
    return True;
  }

  const Ttx::Type& canonical = type.canonical();
  return !canonical.is_invalid() && canonical.get_name() == name;
}

static auto named_spirv_type_id(const Ttx::Type& type) -> SpirvId {
  View::Bytes shader_type =
      type.resolve_attribute(shader_type_attribute).get_bytes();
  SpirvId result =
      ShaderTypeNames::find_or_default(shader_type, SpirvId::Invalid);
  if (result != SpirvId::Invalid) {
    return result;
  }

  result = SpirvTypeNames::find_or_default(type.get_name(), SpirvId::Invalid);
  if (result != SpirvId::Invalid) {
    return result;
  }

  const Ttx::Type& canonical = type.canonical();
  return canonical.is_invalid() || &canonical == &type
             ? SpirvId::Invalid
             : SpirvTypeNames::find_or_default(
                   canonical.get_name(), SpirvId::Invalid);
}

static auto spirv_type_id(const Ttx::Type& type) -> SpirvId {
  // Shader lowering is identity based. A 4x Real_32 layout is not automatically
  // a vec4. It only lowers that way when the TTX type or one of its aliases
  // carries the shader_type fact.
  return named_spirv_type_id(type);
}

static auto pointer_type_id(
    SpirvId type_id,
    Assembler::SpirV::StorageClass storage) -> SpirvId {
  for (Count i = 0; i < core_pointer_decls.get_size(); i++) {
    if (core_pointer_decls[i].storage == storage &&
        core_pointer_decls[i].pointee == type_id) {
      return core_pointer_decls[i].pointer;
    }
  }

  return SpirvId::Invalid;
}

static auto spirv_type_size(SpirvId type_id) -> Unsigned_32 {
  switch (type_id) {
  case SpirvId::Bits32:
  case SpirvId::Real32:
    return 4;

  case SpirvId::Vec2:
  case SpirvId::Uvec2:
    return 8;

  case SpirvId::Vec4:
    return 16;

  default:
    return 0;
  }
}

static auto parameter_id(Count index) -> SpirvId {
  return SpirvId(spirv_id(SpirvId::ParameterBase) + Unsigned_32(index));
}

static auto result_id(Count index) -> SpirvId {
  return SpirvId(spirv_id(SpirvId::ResultBase) + Unsigned_32(index));
}

static auto resource_id(Count index) -> SpirvId {
  return SpirvId(spirv_id(SpirvId::ResourceBase) + Unsigned_32(index));
}

static auto temporary_id(Count index) -> SpirvId {
  return SpirvId(spirv_id(SpirvId::TemporaryBase) + Unsigned_32(index));
}

static auto is_texture_resource(const Ttx::Type& type) -> Bool {
  return type_has_name(type, "Image"_view) ||
         type.find_addressable_function("sample"_view) != nullptr;
}

static auto direct_reference(
    const Tetrodotoxin::Isa::Base::Expression::Value& value) -> Bool {
  return value.get_kind() ==
             Tetrodotoxin::Isa::Base::Expression::Value::Kind::Reference &&
         value.get_tokens().get_size() == 1;
}

static auto carrier_matches_results(
    const Tetrodotoxin::Isa::Base::Expression::Pack& pack,
    View::Vector<Ttx::Member> results) -> Bool {
  View::Vector<Tetrodotoxin::Isa::Base::Expression::Pack::Entry> entries =
      pack.get_entries();
  if (entries.get_size() != results.get_size()) {
    return False;
  }

  for (Count i = 0; i < entries.get_size(); i++) {
    if (entries[i].is_named() &&
        entries[i].get_name() != results[i].get_name()) {
      return False;
    }
  }

  return True;
}

static auto ordered_direct_parameters(
    const Tetrodotoxin::Isa::Base::Expression::Pack& pack,
    View::Vector<Ttx::Member> parameters) -> Bool {
  View::Vector<Tetrodotoxin::Isa::Base::Expression::Pack::Entry> entries =
      pack.get_entries();
  if (entries.get_size() != parameters.get_size()) {
    return False;
  }

  for (Count i = 0; i < entries.get_size(); i++) {
    Tetrodotoxin::Isa::Base::Expression::Value value = entries[i].get_value();
    if (!direct_reference(value) ||
        value.get_value() != parameters[i].get_name()) {
      return False;
    }
  }

  return True;
}

static auto emit_core_types(Assembler::SpirV& assembler) -> void {
  // Core type declarations are the SPIR-V type vocabulary this compiler knows
  // how to produce today. Higher-level TTX names are lowered to these ids
  // before variables or instructions are emitted.
  assembler.type_void(spirv_id(SpirvId::Void));
  assembler.type_function(
      spirv_id(SpirvId::VoidFunction), spirv_id(SpirvId::Void));
  assembler.type_int(spirv_id(SpirvId::Bits32), 32, False);
  assembler.type_float(spirv_id(SpirvId::Real32), 32);
  assembler.type_vector(spirv_id(SpirvId::Vec2), spirv_id(SpirvId::Real32), 2);
  assembler.type_vector(spirv_id(SpirvId::Vec4), spirv_id(SpirvId::Real32), 4);
  assembler.type_vector(spirv_id(SpirvId::Uvec2), spirv_id(SpirvId::Bits32), 2);
  assembler.type_image(
      spirv_id(SpirvId::Image2D), spirv_id(SpirvId::Real32),
      Assembler::SpirV::Dim::D2, 0, 0, 0, 1,
      Assembler::SpirV::ImageFormat::Unknown);
  assembler.type_sampled_image(
      spirv_id(SpirvId::SampledImage), spirv_id(SpirvId::Image2D));
  for (Count i = 0; i < core_pointer_decls.get_size(); i++) {
    assembler.type_pointer(
        spirv_id(core_pointer_decls[i].pointer), core_pointer_decls[i].storage,
        spirv_id(core_pointer_decls[i].pointee));
  }
}

static auto begin_stage_module(
    Assembler::SpirV& assembler,
    Assembler::SpirV::ExecutionModel model,
    View::Vector<Unsigned_32> interface_ids) -> void {
  // A shader module starts with the normal SPIR-V header and declares the entry
  // point before any debug names, decorations, types, variables, or functions.
  // The bound is one greater than every id the fixed stage ranges may emit.
  assembler.begin_module(stage_id_bound);
  assembler.capability(Assembler::SpirV::Capability::Shader);
  assembler.memory_model(
      Assembler::SpirV::AddressingModel::Logical,
      Assembler::SpirV::MemoryModel::GLSL450);
  assembler.entry_point(
      model, spirv_id(SpirvId::EntryFunction), "main"_view, interface_ids);
  if (model == Assembler::SpirV::ExecutionModel::Fragment) {
    assembler.execution_mode(
        spirv_id(SpirvId::EntryFunction),
        Assembler::SpirV::ExecutionMode::OriginUpperLeft);
  }
}

static auto emit_names(
    Assembler::SpirV& assembler,
    View::Vector<Ttx::Member> parameters,
    View::Vector<Ttx::Member> results,
    View::Vector<Ttx::Member> push_members,
    View::Vector<Ttx::Member> resource_members) -> void {
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
    assembler.name(spirv_id(SpirvId::PushVariable), "push"_view);
    for (Count i = 0; i < push_members.get_size(); i++) {
      assembler.member_name(
          spirv_id(SpirvId::PushStruct), Unsigned_32(i),
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
    View::Vector<Ttx::Member> parameters,
    View::Vector<Ttx::Member> results) -> void {
  // Render stage signatures become the shader entry interface. The current
  // naming convention maps vertex_index and screen_position to Vulkan builtins.
  // The remaining stage handoff values use dense locations.
  Count input_location = 0;
  for (Count i = 0; i < parameters.get_size(); i++) {
    if (model == Assembler::SpirV::ExecutionModel::Vertex &&
        parameters[i].get_name() == "vertex_index"_view) {
      assembler.decorate(
          spirv_id(parameter_id(i)), Assembler::SpirV::Decoration::BuiltIn,
          Unsigned_32(Assembler::SpirV::BuiltIn::VertexIndex));
      continue;
    }

    assembler.decorate(
        spirv_id(parameter_id(i)), Assembler::SpirV::Decoration::Location,
        Unsigned_32(input_location++));
  }

  Count output_location = 0;
  for (Count i = 0; i < results.get_size(); i++) {
    if (model == Assembler::SpirV::ExecutionModel::Vertex &&
        results[i].get_name() == "screen_position"_view) {
      assembler.decorate(
          spirv_id(result_id(i)), Assembler::SpirV::Decoration::BuiltIn,
          Unsigned_32(Assembler::SpirV::BuiltIn::Position));
      continue;
    }

    assembler.decorate(
        spirv_id(result_id(i)), Assembler::SpirV::Decoration::Location,
        Unsigned_32(output_location++));
  }
}

static auto emit_stage_storage_decorations(
    Assembler::SpirV& assembler,
    View::Vector<SpirvId> push_member_type_ids,
    View::Vector<Ttx::Member> resource_members) -> void {
  // Push and resource facts are owned by Render. Shader lowering only emits the
  // storage surfaces the stage declared it can read.
  if (!push_member_type_ids.is_empty()) {
    assembler.decorate(
        spirv_id(SpirvId::PushStruct), Assembler::SpirV::Decoration::Block);
    Unsigned_32 offset = 0;
    for (Count i = 0; i < push_member_type_ids.get_size(); i++) {
      assembler.member_decorate(
          spirv_id(SpirvId::PushStruct), Unsigned_32(i),
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
        Unsigned_32(i));
  }
}

static auto emit_push_type(
    Allocator::Arena& arena,
    Assembler::SpirV& assembler,
    View::Vector<SpirvId> push_member_type_ids) -> void {
  // Push constants are represented as a Block-decorated struct plus a pointer
  // to that struct in the PushConstant storage class. The offsets were emitted
  // in the annotation phase. This phase declares the actual structural type.
  if (push_member_type_ids.is_empty()) {
    return;
  }

  Managed::Vector<Unsigned_32> push_member_type_words(arena);
  for (Count i = 0; i < push_member_type_ids.get_size(); i++) {
    push_member_type_words.insert(spirv_id(push_member_type_ids[i]));
  }

  assembler.type_struct(
      spirv_id(SpirvId::PushStruct), push_member_type_words.get_view());
  assembler.type_pointer(
      spirv_id(SpirvId::PushPointer),
      Assembler::SpirV::StorageClass::PushConstant,
      spirv_id(SpirvId::PushStruct));
}

static auto emit_variables(
    Assembler::SpirV& assembler,
    View::Vector<Ttx::Member> parameters,
    View::Vector<Ttx::Member> results,
    Bool has_push,
    View::Vector<Ttx::Member> resource_members) -> void {
  // Variables bind the interface ids from OpEntryPoint to concrete pointer
  // types and storage classes. Shader inputs, outputs, push constants, and
  // resources are all global variables from SPIR-V's point of view.
  for (Count i = 0; i < parameters.get_size(); i++) {
    SpirvId type_id = spirv_type_id(parameters[i].get_type());
    assembler.variable(
        spirv_id(
            pointer_type_id(type_id, Assembler::SpirV::StorageClass::Input)),
        spirv_id(parameter_id(i)), Assembler::SpirV::StorageClass::Input);
  }

  for (Count i = 0; i < results.get_size(); i++) {
    SpirvId type_id = spirv_type_id(results[i].get_type());
    assembler.variable(
        spirv_id(
            pointer_type_id(type_id, Assembler::SpirV::StorageClass::Output)),
        spirv_id(result_id(i)), Assembler::SpirV::StorageClass::Output);
  }

  if (has_push) {
    assembler.variable(
        spirv_id(SpirvId::PushPointer), spirv_id(SpirvId::PushVariable),
        Assembler::SpirV::StorageClass::PushConstant);
  }

  for (Count i = 0; i < resource_members.get_size(); i++) {
    assembler.variable(
        spirv_id(SpirvId::ResourcePointer), spirv_id(resource_id(i)),
        Assembler::SpirV::StorageClass::UniformConstant);
  }
}

static auto shader_block(
    const Tetrodotoxin::Isa::Base::Implementation& implementation,
    const Ttx::Function& stage) -> const Tetrodotoxin::Isa::Shader::Block* {
  return implementation.find<Tetrodotoxin::Isa::Shader::Block>(stage);
}

static auto shader_contract(const Ttx::Type& shader) -> const Ttx::Type* {
  const Ttx::Type* contract_alias = shader.find_type("Contract"_view);
  if (contract_alias == nullptr || !contract_alias->is_alias()) {
    return nullptr;
  }

  const Ttx::Type& contract = contract_alias->canonical();
  return contract.is_invalid() ? nullptr : &contract;
}

static auto report(
    Ttx::Lexical::Errors& errors,
    Ttx::Lexical::Source source,
    View::Bytes message,
    View::Bytes hint = View::Bytes()) -> Bool {
  errors.insert(source, message, hint);
  return False;
}

static auto report_statement(
    Ttx::Lexical::Errors& errors,
    Ttx::Lexical::Source source,
    const Tetrodotoxin::Isa::Shader::Statement& statement,
    View::Bytes message,
    View::Bytes hint = View::Bytes()) -> Bool {
  errors.insert_range(
      statement.get_start_token(), statement.get_end_token(), source, message,
      hint);
  return False;
}

static auto emit_return_pack(
    Allocator::Arena& arena,
    Ttx::Lexical::Errors& errors,
    Ttx::Lexical::Source source,
    Assembler::SpirV& assembler,
    const Tetrodotoxin::Isa::Shader::Statement& statement,
    const Tetrodotoxin::Isa::Base::Expression::Pack& pack,
    View::Vector<Ttx::Member> parameters,
    View::Vector<Ttx::Member> results,
    Count& temporary_count) -> Bool {
  View::Vector<Tetrodotoxin::Isa::Base::Expression::Pack::Entry> entries =
      pack.get_entries();
  Ttx::Layout parameter_layout(parameters);
  Ttx::Layout result_layout(results);
  Managed::Vector<Ttx::Member> value_schema(arena);
  for (Count i = 0; i < entries.get_size(); i++) {
    Tetrodotoxin::Isa::Base::Expression::Value value = entries[i].get_value();
    if (!direct_reference(value)) {
      return report_statement(
          errors, source, statement,
          "Only direct shader return references can lower today."_view);
    }

    const Ttx::Member* parameter =
        parameter_layout.find_member(value.get_value());
    if (parameter == nullptr) {
      return report_statement(
          errors, source, statement,
          "Shader return reference did not resolve to a parameter."_view);
    }

    value_schema.insert(*parameter);
  }

  Ttx::Layout return_schema = pack.schema(arena, value_schema.get_view());
  if (!return_schema.fits(result_layout)) {
    return report_statement(
        errors, source, statement,
        "Shader return pack does not match stage result."_view);
  }

  if (!carrier_matches_results(pack, results) ||
      !ordered_direct_parameters(pack, parameters)) {
    return report_statement(
        errors, source, statement,
        "Only ordered shader return pass-through can lower today."_view);
  }

  for (Count i = 0; i < entries.get_size(); i++) {
    SpirvId type_id = spirv_type_id(results[i].get_type());
    SpirvId value_id = temporary_id(temporary_count++);
    assembler.load(
        spirv_id(type_id), spirv_id(value_id), spirv_id(parameter_id(i)));
    assembler.store(spirv_id(result_id(i)), spirv_id(value_id));
  }

  return True;
}

static auto emit_shader_block(
    Allocator::Arena& arena,
    Ttx::Lexical::Errors& errors,
    Ttx::Lexical::Source source,
    Assembler::SpirV& assembler,
    const Tetrodotoxin::Isa::Shader::Block& block,
    View::Vector<Ttx::Member> parameters,
    View::Vector<Ttx::Member> results) -> Bool {
  Count temporary_count = 0;
  Bool block_valid = True;
  View::Vector<Tetrodotoxin::Isa::Shader::Statement> statements =
      block.get_statements();
  for (Count i = 0; i < statements.get_size(); i++) {
    Tetrodotoxin::Isa::Shader::Statement::Kind kind = statements[i].get_kind();
    if (kind == Tetrodotoxin::Isa::Shader::Statement::Kind::State) {
      report_statement(
          errors, source, statements[i],
          "Shader state statements cannot lower today."_view,
          "State expressions need SSA lowering before SPIR-V emission."_view);
      block_valid = False;
      continue;
    }

    if (kind == Tetrodotoxin::Isa::Shader::Statement::Kind::BareReturn) {
      return block_valid;
    }

    if (kind == Tetrodotoxin::Isa::Shader::Statement::Kind::Empty) {
      return report(
          errors, source,
          "Shader compiler received an empty durable statement."_view);
    }

    Bool return_valid = emit_return_pack(
        arena, errors, source, assembler, statements[i],
        statements[i].get_return_pack(), parameters, results, temporary_count);
    return block_valid && return_valid;
  }

  return block_valid;
}

static auto emit_entry_point(
    Allocator::Arena& arena,
    Ttx::Lexical::Errors& errors,
    Ttx::Lexical::Source source,
    Assembler::SpirV& assembler,
    const Tetrodotoxin::Isa::Shader::Block* block,
    View::Vector<Ttx::Member> parameters,
    View::Vector<Ttx::Member> results) -> Bool {
  // Shader entry points return void. Stage results are written through Output
  // variables, so body lowering emits stores before the final OpReturn.
  assembler.function(
      spirv_id(SpirvId::Void), spirv_id(SpirvId::EntryFunction),
      Assembler::SpirV::FunctionControl::None, spirv_id(SpirvId::VoidFunction));
  assembler.label(spirv_id(SpirvId::EntryLabel));
  if (block != nullptr) {
    Bool emitted = emit_shader_block(
        arena, errors, source, assembler, *block, parameters, results);
    if (!emitted) {
      return False;
    }
  }

  assembler.return_void();
  assembler.function_end();
  return True;
}

auto Isa::Shader::Compiler::lower(
    Allocator::Arena& arena,
    Ttx::Lexical::Errors& errors,
    Ttx::Lexical::Source source,
    View::Bytes module,
    const Ttx::Type& root,
    const Tetrodotoxin::Isa::Base::Implementation& implementation) -> Bool {
  const Ttx::Type* contract = shader_contract(root);
  if (contract == nullptr) {
    return report(
        errors, source, "Shader compiler could not find render contract."_view);
  }

  View::Vector<Ttx::Function> stages = root.get_type_functions();
  if (stages.is_empty()) {
    return report(
        errors, source, "Shader source did not declare any stages."_view);
  }

  Bool valid = True;
  for (Count i = 0; i < stages.get_size(); i++) {
    Bool lowered = lower_stage(
        arena, errors, source, module, root, *contract, stages[i],
        implementation);
    if (!lowered) {
      valid = False;
    }
  }

  return valid;
}

auto Isa::Shader::Compiler::lower_stage(
    Allocator::Arena& arena,
    Ttx::Lexical::Errors& errors,
    Ttx::Lexical::Source source,
    View::Bytes module,
    const Ttx::Type& shader,
    const Ttx::Type& contract,
    const Ttx::Function& stage,
    const Tetrodotoxin::Isa::Base::Implementation& implementation) -> Bool {
  constexpr Assembler::SpirV::ExecutionModel invalid_model =
      Assembler::SpirV::ExecutionModel(Count(-1));
  Assembler::SpirV::ExecutionModel model =
      ExecutionModels::find_or_default(stage.get_name(), invalid_model);
  if (model == invalid_model) {
    return report(
        errors, source,
        "Only vertex and pixel shader stages can lower today."_view);
  }

  // Render owns the pipeline contract. Shader lowering consumes the same stage
  // facts the ISA validator already checked instead of creating a compiler-side
  // mirror for push constants and resources.
  const Ttx::Type* stage_facts = contract.find_type(stage.get_name());
  if (stage_facts == nullptr) {
    return report(
        errors, source, "Shader stage has no render fact contract."_view);
  }

  const Ttx::Type* push = stage_facts->find_type("push"_view);
  const Ttx::Type* resource = stage_facts->find_type("resource"_view);
  View::Vector<Ttx::Member> parameters = stage.get_parameters().get_members();
  View::Vector<Ttx::Member> results = stage.get_result().get_members();
  View::Vector<Ttx::Member> push_members =
      push == nullptr ? View::Vector<Ttx::Member>() : push->get_members();
  View::Vector<Ttx::Member> resource_members = resource == nullptr
                                                   ? View::Vector<Ttx::Member>()
                                                   : resource->get_members();

  Managed::Vector<SpirvId> push_member_type_ids(arena);
  for (Count i = 0; i < push_members.get_size(); i++) {
    SpirvId type_id = spirv_type_id(push_members[i].get_type());
    if (type_id == SpirvId::Invalid) {
      return report(
          errors, source, "Shader push constant type cannot lower today."_view);
    }

    push_member_type_ids.insert(type_id);
  }

  for (Count i = 0; i < resource_members.get_size(); i++) {
    if (!is_texture_resource(resource_members[i].get_type())) {
      return report(
          errors, source, "Shader resource type cannot lower today."_view);
    }
  }

  Managed::Vector<Unsigned_32> interface_ids(arena);
  for (Count i = 0; i < parameters.get_size(); i++) {
    SpirvId type_id = spirv_type_id(parameters[i].get_type());
    if (type_id == SpirvId::Invalid) {
      return report(
          errors, source, "Shader parameter type cannot lower today."_view);
    }

    interface_ids.insert(spirv_id(parameter_id(i)));
  }

  for (Count i = 0; i < results.get_size(); i++) {
    SpirvId type_id = spirv_type_id(results[i].get_type());
    if (type_id == SpirvId::Invalid) {
      return report(
          errors, source, "Shader result type cannot lower today."_view);
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
  emit_push_type(arena, assembler, push_member_type_ids.get_view());
  emit_variables(
      assembler, parameters, results, !push_member_type_ids.is_empty(),
      resource_members);
  Bool entry_emitted = emit_entry_point(
      arena, errors, source, assembler, shader_block(implementation, stage),
      parameters, results);
  if (!entry_emitted) {
    return False;
  }

  if (!Assembler::SpirV::is_valid_module(words)) {
    return report(
        errors, source, "Shader compiler emitted invalid SPIR-V."_view);
  }

  const Count offset = read_only.get_size();
  read_only.concat(words.get_view());
  stages.insert(Symbol(
      stage_name(arena, module, shader.get_name(), stage.get_name()),
      {offset, words.get_size()}));
  return True;
}

auto Isa::Shader::Compiler::stage_name(
    Allocator::Arena& arena,
    View::Bytes module,
    View::Bytes shader,
    View::Bytes stage) -> View::Bytes {
  Managed::Bytes output(arena);
  output.concat("TTX_shader_"_view);
  append_name_segment(output, module);
  output.append('_');
  output.concat(shader);
  output.append('_');
  output.concat(stage);
  output.concat("_spirv"_view);
  return output;
}

auto Isa::Shader::Compiler::append_name_segment(
    Managed::Bytes& output,
    View::Bytes value) -> void {
  // Module names are derived from source filenames, not TTX identifiers, so the
  // exported stage name needs a normalized module segment.
  for (Count i = 0; i < value.get_size(); i++) {
    Unsigned_8 c = value[i];
    const Bool alpha = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
    const Bool digit = c >= '0' && c <= '9';
    output.append(alpha || digit || c == '_' ? c : Unsigned_8('_'));
  }
}
