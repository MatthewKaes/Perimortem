// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/shader/language/contract.hpp"

#include "tetrodotoxin/library/language/function.hpp"
#include "tetrodotoxin/render/language/attributes.hpp"
#include "tetrodotoxin/render/language/binding.hpp"
#include "tetrodotoxin/render/language/stage.hpp"
#include "ttx/model/interfaces/callable.hpp"

using namespace Perimortem::Core;
using namespace Ttx::Concept;
using namespace Tetrodotoxin;
using namespace Tetrodotoxin::Shader;

static auto find_function(
    const Shader::Language::Program& program,
    View::Bytes name) -> Option<const Library::Language::Function&> {
  for (const Reference<Abstract>& candidate : program.get_callables()) {
    auto function = candidate.get().select<Library::Language::Function>();
    if (function && function->get_name() == name) {
      return *function;
    }
  }
  return {};
}

static auto find_binding(
    const Shader::Language::Program& program,
    View::Bytes name) -> Option<const Shader::Language::Binding&> {
  auto bindings = program.get_bindings();
  for (Count index = 0; index < bindings.get_size(); index++) {
    const Shader::Language::Binding& binding = bindings.get_data()[index];
    if (binding.get_field().get_name() == name) {
      return binding;
    }
  }
  return {};
}

static auto slots_satisfy(
    const Library::Language::Model::Layout& supplied,
    const Render::Language::Layout& required) -> Bool {
  for (const Render::Language::Layout::Slot& requirement :
       required.get_slots()) {
    Option<Count> selected;
    for (Count index = 0; index < supplied.get_size(); index++) {
      auto name = supplied.get_name(index);
      if (name && *name == requirement.get_name()) {
        selected = index;
        break;
      }
    }
    BAIL_IF(
        !selected || !Render::Language::Attributes::satisfies(
                         supplied.get_slot_attributes(*selected),
                         requirement.get_attributes()));
  }
  return True;
}

auto Shader::Language::Contract::negotiate(
    const Abstract& requirement,
    const Abstract& candidate) const -> Relation {
  auto render = requirement.resolve().select<Render::Language::Structure>();
  auto shader = candidate.resolve().select<Shader::Language::Program>();
  BAIL_IF(!render || !shader);

  // Callable negotiation establishes the shared value flow first. Render then
  // restores Stage and slot policy that Layout fitting deliberately omits.
  Ttx::Model::Interfaces::Callable callable_interface;
  for (const Reference<Abstract>& entry : render->get_callables()) {
    auto required = entry.get().select<Render::Language::Stage>();
    BAIL_IF(!required);
    auto supplied = find_function(*shader, required->get_name());
    BAIL_IF(
        !supplied || !callable_interface.accepts(*required, *supplied) ||
        !Render::Language::Attributes::satisfies(
            supplied->get_definition().get_attributes(),
            required->get_definition().get_attributes()) ||
        !slots_satisfy(
            supplied->get_signature().get_parameters(),
            required->get_parameter_layout()) ||
        !slots_satisfy(
            supplied->get_signature().get_results(),
            required->get_result_layout()));
  }

  // Bindings preserve exact Type identity because a matching storage shape is
  // not enough to select a CPU and GPU relationship. Bridge remains the owner
  // when two distinct Types intentionally correspond.
  for (const Reference<Abstract>& entry : render->get_addressables()) {
    auto required = entry.get().select<Render::Language::Binding>();
    BAIL_IF(!required);
    auto supplied = find_binding(*shader, required->get_name());
    BAIL_IF(!supplied);
    const Library::Language::Field& field = supplied->get_field();
    auto required_definition = required->get_definition();
    BAIL_IF(
        supplied->get_kind() != required->get_kind() ||
        &field.get_type() != &required->get_type() ||
        (required_definition && !Render::Language::Attributes::satisfies(
                                    field.get_definition().get_attributes(),
                                    required_definition->get_attributes())));
  }

  return Relation::Satisfied;
}

auto Shader::Language::Contract::validate(
    Ttx::Lexical::Cursor& cursor,
    const Abstract& requirement,
    const Abstract& candidate) const -> Bool {
  Relation relation = negotiate(requirement, candidate);
  if (relation != Relation::Rejected) {
    return True;
  }

  auto program = candidate.select<Shader::Language::Program>();
  cursor.create_expression_error(
      program ? program->get_anchor()
              : Ttx::Lexical::Anchor::create(Ttx::Lexical::Span()),
      "Shader Program does not satisfy its selected Render contract."_view,
      "Match every required Stage, value, Type, and Render Attribute."_view);
  return False;
}
