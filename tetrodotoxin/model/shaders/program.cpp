// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/model/shaders/program.hpp"

#include "tetrodotoxin/model/callables/sample.hpp"
#include "tetrodotoxin/model/shaders/binary.hpp"
#include "ttx/concept/invalid.hpp"
#include "ttx/model/types/managed.hpp"
#include "ttx/model/types/real.hpp"
#include "ttx/model/types/unsigned.hpp"

using namespace Perimortem::Core;
using namespace Ttx::Concept;

static auto is_render_resource(
    const Tetrodotoxin::Model::Render& render,
    const Ttx::Model::Addressable& addressable) -> Bool {
  for (Count i = 0; i < render.get_resource_count(); i++) {
    if (&render.get_resource(i) == &addressable) {
      return True;
    }
  }
  return False;
}

// Managed references are host/runtime values. The only managed value admitted
// to a Shader Body is an opaque load from a real Render resource role. Managed
// parameters, results, constants, pushes, locals, and aggregates fail closed.
static auto body_types_legal(
    const Ttx::Model::Body& body,
    const Tetrodotoxin::Model::Render& render) -> Bool {
  auto values = body.get_values();
  auto operations = body.get_operations();
  for (Count i = 0; i < values.get_size(); i++) {
    const Ttx::Model::Bodies::Value& value = values[i];
    if (!value.get_type().is<Ttx::Model::Types::Managed>()) {
      continue;
    }

    if (value.is_parameter() ||
        value.get_operation() >= operations.get_size()) {
      return False;
    }

    Bool legal_resource_load = operations[value.get_operation()].visit(
        []() -> Bool { return False; },
        [&](const Ttx::Model::Bodies::Operations::Load& load) -> Bool {
          return is_render_resource(render, load.get_addressable());
        },
        [](const auto&) -> Bool { return False; });
    if (!legal_resource_load) {
      return False;
    }
  }

  return True;
}

static auto body_operations_legal(const Ttx::Model::Body& body) -> Bool {
  const auto values = body.get_values();
  const auto operations = body.get_operations();
  for (Count i = 0; i < operations.get_size(); i++) {
    Bool legal = operations[i].visit(
        []() -> Bool { return False; },
        [&](const Ttx::Model::Bodies::Operations::Binary& operation) -> Bool {
          const Ttx::Model::Type& left =
              values[operation.get_left().get_value()].get_type();
          const Ttx::Model::Type& right =
              values[operation.get_right().get_value()].get_type();
          const Ttx::Model::Type& result =
              values[operation.get_result().get_value()].get_type();
          if (!Tetrodotoxin::Model::Operations::Binary::is_valid(operation)) {
            return False;
          }

          const Abstract& expected =
              Tetrodotoxin::Model::Shaders::Binary::resolve(
                  Tetrodotoxin::Model::Operations::Binary::get_operator(
                      operation),
                  left, right);
          return expected.is<Ttx::Model::Type>() &&
                 &expected.resolve() == &result.resolve();
        },
        [&](const Ttx::Model::Bodies::Operations::Convert& operation) -> Bool {
          const Ttx::Model::Type& source =
              values[operation.get_source().get_value()].get_type();
          const Ttx::Model::Type& result =
              values[operation.get_result().get_value()].get_type();
          return source.is<Ttx::Model::Types::Unsigned>() &&
                 result.is<Ttx::Model::Types::Real>();
        },
        [](const Ttx::Model::Bodies::Operations::IndexedWrite&) -> Bool {
          return False;
        },
        [](const Ttx::Model::Bodies::Operations::Call& operation) -> Bool {
          return operation.get_callable()
              .is<Tetrodotoxin::Model::Callables::Sample>();
        },
        [](const auto&) -> Bool { return True; });
    if (!legal) {
      return False;
    }
  }

  return True;
}

auto Tetrodotoxin::Model::Shaders::Program::construct(
    Perimortem::Memory::Allocator::Arena& arena,
    View::Bytes name,
    const Model::Render& render,
    View::Vector<Reference<Model::Stages::Implemented>> stages,
    const Documentation& documentation) -> const Abstract& {
  Program* storage = arena.reserve<Program>();
  new (storage) Program(arena, name, render, stages, documentation);
  return storage->valid ? static_cast<const Abstract&>(*storage)
                        : Invalid::get_invalid();
}

Tetrodotoxin::Model::Shaders::Program::Program(
    Perimortem::Memory::Allocator::Arena& arena,
    View::Bytes name,
    const Model::Render& render,
    View::Vector<Reference<Model::Stages::Implemented>> stages,
    const Documentation& documentation)
    : name(arena.proxy(name)),
      render(render),
      documentation(documentation),
      stages(arena),
      stages_by_name(arena) {
  if (name.is_empty() || stages.get_size() != render.get_stage_count()) {
    valid = False;
    return;
  }
  this->stages.reset(stages.get_size());
  for (Count i = 0; i < stages.get_size(); i++) {
    const Model::Stages::Implemented& stage = stages[i].get();
    const Abstract& required = render.resolve_context(stage.get_name());
    if (!required.is<Model::Stages::Required>() ||
        &stage.get_required() != &required ||
        !stage.get_parameters().fits(
            required.assume<Ttx::Model::Callable>().get_parameters()) ||
        !stage.get_results().fits(
            required.assume<Ttx::Model::Callable>().get_results()) ||
        !stage.get_body().is_valid() ||
        !body_types_legal(stage.get_body(), render) ||
        !body_operations_legal(stage.get_body()) ||
        stages_by_name.find(stage.get_name()) != nullptr) {
      valid = False;
      return;
    }
    this->stages.insert(stages[i]);
    stages_by_name.insert(stage.get_name(), stages[i]);
  }
  for (Count i = 0; i < render.get_stage_count(); i++) {
    const Abstract& required = render.get_stage(i);
    if (!required.is<Model::Stages::Required>() ||
        stages_by_name.find(required.get_name()) == nullptr) {
      valid = False;
      return;
    }
  }
}

auto Tetrodotoxin::Model::Shaders::Program::resolve_context(
    View::Bytes route) const -> const Abstract& {
  const auto* selected = stages_by_name.find(route);
  if (selected == nullptr) {
    return Invalid::get_invalid();
  }
  return selected->value.get();
}

auto Tetrodotoxin::Model::Shaders::Program::get_stage(Count index) const
    -> const Abstract& {
  if (index >= stages.get_size()) {
    return Invalid::get_invalid();
  }
  return stages.get_view()[index].get();
}
