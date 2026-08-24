// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/terminal/spirv/module/interface.hpp"

#include "tetrodotoxin/library/language/model/addressable.hpp"
#include "tetrodotoxin/render/language/attributes.hpp"
#include "tetrodotoxin/render/language/stage.hpp"

using namespace Perimortem;
using namespace Tetrodotoxin;
using namespace Tetrodotoxin::Terminal::Spirv;

static auto attribute(
    Core::View::Vector<Tetrodotoxin::Language::Attribute> attributes,
    Core::View::Bytes name)
    -> Core::Option<const Tetrodotoxin::Language::Attribute&> {
  for (Count index = 0; index < attributes.get_size(); index++) {
    const Tetrodotoxin::Language::Attribute& selected =
        attributes.get_data()[index];
    if (selected.get_key() == name) {
      return selected;
    }
  }
  return {};
}

static auto execution_model(
    Core::View::Bytes stage_name,
    Core::View::Vector<Tetrodotoxin::Language::Attribute> attributes)
    -> Core::Option<Assembler::SpirV::ExecutionModel> {
  Core::View::Bytes name = stage_name;
  auto capability = attribute(attributes, "capability"_view);
  if (capability) {
    const Core::View::Bytes* selected =
        capability->get_value().find<Core::View::Bytes>();
    BAIL_IF(!selected);
    name = *selected;
  }
  if (name == "vertex"_view) {
    return Assembler::SpirV::ExecutionModel::Vertex;
  }
  if (name == "fragment"_view) {
    return Assembler::SpirV::ExecutionModel::Fragment;
  }
  return {};
}

auto Module::Interface::prepare(const Shader::Language::Program& program)
    -> Bool {
  // Render chooses which Functions are entry points. Starting from its required
  // Stages prevents a helper Function from becoming an accidental GPU entry
  // merely because it shares the Program context.
  auto contract = program.get_contract();
  BAIL_IF(!contract);
  for (const Ttx::Concept::Reference<Ttx::Concept::Abstract>& requirement :
       contract->get_callables()) {
    auto render_stage = requirement.get().select<Render::Language::Stage>();
    BAIL_IF(!render_stage);
    Core::Option<const Library::Language::Function&> function;
    for (const Ttx::Concept::Reference<Ttx::Concept::Abstract>& candidate :
         program.get_callables()) {
      auto selected = candidate.get().select<Library::Language::Function>();
      if (selected && selected->get_name() == render_stage->get_name()) {
        BAIL_IF(function);
        function = *selected;
      }
    }
    BAIL_IF(!function || function->declares_self() || !function->get_body());
    auto model = execution_model(
        render_stage->get_name(),
        render_stage->get_definition().get_attributes());
    BAIL_IF(!model);

    auto& stage = arena.construct<Stage>(arena, *function, *model, ids.take());
    BAIL_IF(
        !prepare_variables(
            stage, function->get_signature().get_parameters(),
            Assembler::SpirV::StorageClass::Input) ||
        !prepare_variables(
            stage, function->get_signature().get_results(),
            Assembler::SpirV::StorageClass::Output));
    stages.insert(&stage);
  }
  return !stages.is_empty();
}

auto Module::Interface::prepare_variables(
    Stage& stage,
    const Library::Language::Model::Layout& layout,
    Assembler::SpirV::StorageClass storage) -> Bool {
  // The Function Layout supplies exact value Types while its validated Render
  // Attributes retain the interface location or builtin meaning.
  for (Count index = 0; index < layout.get_size(); index++) {
    auto semantic = layout.get_abstract(index);
    auto type = semantic
                    ? Types::select(*semantic)
                    : Core::Option<const Library::Language::Model::Type&>();
    Core::View::Bytes name = layout.get_declared_name(index);
    auto attributes = layout.get_slot_attributes(index);
    BAIL_IF(
        !semantic || !type || name.is_empty() ||
        !Render::Language::Attributes::accepts(
            attributes, Render::Language::Attributes::Placement::StageEntry) ||
        !types.collect_pointer(*type, storage));
    Variable variable(*semantic, *type, name, attributes, storage, ids.take());
    if (storage == Assembler::SpirV::StorageClass::Input) {
      stage.inputs.insert(variable);
    } else {
      stage.outputs.insert(variable);
    }
  }
  return True;
}

auto Module::Interface::emit_entry_points(Assembler::SpirV& assembler) const
    -> void {
  for (Stage* stage : stages.get_view()) {
    Memory::Dynamic::Vector<U32> variables;
    for (const Variable& input : stage->inputs.get_view()) {
      variables.insert(input.id);
    }
    for (const Variable& output : stage->outputs.get_view()) {
      variables.insert(output.id);
    }
    assembler.entry_point(
        stage->model, stage->id, stage->function.get().get_name(),
        variables.get_view());
    if (stage->model == Assembler::SpirV::ExecutionModel::Fragment) {
      assembler.execution_mode(
          stage->id, Assembler::SpirV::ExecutionMode::OriginUpperLeft);
    }
  }
}

auto Module::Interface::emit_debug(Assembler::SpirV& assembler) const -> void {
  for (Stage* stage : stages.get_view()) {
    assembler.name(stage->id, stage->function.get().get_name());
    for (const Variable& input : stage->inputs.get_view()) {
      assembler.name(input.id, input.name);
    }
    for (const Variable& output : stage->outputs.get_view()) {
      assembler.name(output.id, output.name);
    }
  }
}

auto Module::Interface::decorate(
    Assembler::SpirV& assembler,
    const Variable& variable) -> Bool {
  auto location = attribute(variable.attributes, "location"_view);
  auto builtin = attribute(variable.attributes, "builtin"_view);
  BAIL_IF(Bool(location) == Bool(builtin));
  if (location) {
    const U64* value = location->get_value().find<U64>();
    BAIL_IF(!value || *value > U32(-1));
    assembler.decorate(
        variable.id, Assembler::SpirV::Decoration::Location, U32(*value));
    return True;
  }

  const Core::View::Bytes* name =
      builtin->get_value().find<Core::View::Bytes>();
  BAIL_IF(!name);
  Assembler::SpirV::BuiltIn selected;
  if (*name == "position"_view) {
    selected = Assembler::SpirV::BuiltIn::Position;
  } else if (*name == "vertex_index"_view) {
    selected = Assembler::SpirV::BuiltIn::VertexIndex;
  } else {
    return False;
  }
  assembler.decorate(
      variable.id, Assembler::SpirV::Decoration::BuiltIn, U32(selected));
  return True;
}

auto Module::Interface::emit_annotations(Assembler::SpirV& assembler) const
    -> Bool {
  for (Stage* stage : stages.get_view()) {
    for (const Variable& input : stage->inputs.get_view()) {
      BAIL_IF(!decorate(assembler, input));
    }
    for (const Variable& output : stage->outputs.get_view()) {
      BAIL_IF(!decorate(assembler, output));
    }
  }
  return True;
}

auto Module::Interface::emit_globals(Assembler::SpirV& assembler) const
    -> Bool {
  for (Stage* stage : stages.get_view()) {
    for (const Variable& input : stage->inputs.get_view()) {
      auto pointer = types.get_pointer_id(input.type.get(), input.storage);
      BAIL_IF(!pointer);
      assembler.variable(*pointer, input.id, input.storage);
    }
    for (const Variable& output : stage->outputs.get_view()) {
      auto pointer = types.get_pointer_id(output.type.get(), output.storage);
      BAIL_IF(!pointer);
      assembler.variable(*pointer, output.id, output.storage);
    }
  }
  return True;
}
