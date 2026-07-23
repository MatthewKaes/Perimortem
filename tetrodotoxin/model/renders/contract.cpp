// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/model/renders/contract.hpp"

#include "tetrodotoxin/model/renderables/value.hpp"
#include "tetrodotoxin/model/stages/required.hpp"
#include "ttx/concept/invalid.hpp"

using namespace Perimortem::Core;
using namespace Ttx::Concept;

auto Tetrodotoxin::Model::Renders::Contract::construct(
    Perimortem::Memory::Allocator::Arena& arena,
    View::Bytes name,
    View::Vector<Reference<Model::Renderables::Value>> values,
    const Model::Namespace& constants,
    const Model::Namespace& pushes,
    const Model::Namespace& resources,
    View::Vector<Reference<Model::Stages::Required>> stages,
    const Documentation& documentation) -> const Abstract& {
  Contract* storage = arena.reserve<Contract>();
  new (storage) Contract(
      arena, name, values, constants, pushes, resources, stages, documentation);
  return storage->valid ? static_cast<const Abstract&>(*storage)
                        : Invalid::get_invalid();
}

Tetrodotoxin::Model::Renders::Contract::Contract(
    Perimortem::Memory::Allocator::Arena& arena,
    View::Bytes name,
    View::Vector<Reference<Model::Renderables::Value>> values,
    const Model::Namespace& constants,
    const Model::Namespace& pushes,
    const Model::Namespace& resources,
    View::Vector<Reference<Model::Stages::Required>> stages,
    const Documentation& documentation)
    : name(arena.proxy(name)),
      documentation(documentation),
      members(arena),
      values(arena),
      constants(constants),
      pushes(pushes),
      resources(resources),
      stages(arena),
      layout_values(arena) {
  if (name.is_empty()) {
    valid = False;
    return;
  }
  this->values.reset(values.get_size());
  layout_values.reset(values.get_size());
  for (Count i = 0; i < values.get_size(); i++) {
    Bool added = members.add_export(values[i].get());
    if (!added) {
      valid = False;
      return;
    }
    this->values.insert(values[i]);
    layout_values.insert(Reference<Ttx::Model::Addressable>(values[i].get()));
  }
  Bool constants_added = members.add_export(constants);
  if (!constants_added) {
    valid = False;
    return;
  }
  Bool pushes_added = members.add_export(pushes);
  if (!pushes_added) {
    valid = False;
    return;
  }
  Bool resources_added = members.add_export(resources);
  if (!resources_added) {
    valid = False;
    return;
  }
  this->stages.reset(stages.get_size());
  for (Count i = 0; i < stages.get_size(); i++) {
    Bool added = members.add_export(stages[i].get());
    if (!added) {
      valid = False;
      return;
    }
    this->stages.insert(stages[i]);
  }

  Bool sealed = members.seal();
  if (!sealed) {
    valid = False;
    return;
  }

  layout = Ttx::Model::Layouts::Structured(layout_values.get_view());
}

auto Tetrodotoxin::Model::Renders::Contract::resolve_context(
    View::Bytes route) const -> const Abstract& {
  return members.resolve_context(route);
}

auto Tetrodotoxin::Model::Renders::Contract::get_constant(Count index) const
    -> const Abstract& {
  return constants.get_export(index);
}

auto Tetrodotoxin::Model::Renders::Contract::get_push(Count index) const
    -> const Abstract& {
  return pushes.get_export(index);
}

auto Tetrodotoxin::Model::Renders::Contract::get_resource(Count index) const
    -> const Abstract& {
  return resources.get_export(index);
}

auto Tetrodotoxin::Model::Renders::Contract::get_stage(Count index) const
    -> const Abstract& {
  if (index >= stages.get_size()) {
    return Invalid::get_invalid();
  }
  return stages.get_view()[index].get();
}
