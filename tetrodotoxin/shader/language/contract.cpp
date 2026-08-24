// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/shader/language/contract.hpp"

#include "perimortem/core/diagnostics/log.hpp"

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

static auto slot_failure(
    const Library::Language::Model::Layout& supplied,
    const Render::Language::Layout& required) -> View::Bytes {
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
    if (!selected) {
      return "The restored Stage Layout is missing one named Render slot."_view;
    }
    if (!Render::Language::Attributes::satisfies(
            supplied.get_slot_attributes(*selected),
            requirement.get_attributes())) {
      return "The restored Stage Layout no longer retains its required Render Attributes."_view;
    }
  }
  return {};
}

class Evaluation {
 public:
  constexpr Evaluation(
      Ttx::Concept::Interface::Relation relation,
      View::Bytes failure = {})
      : relation(relation), failure(failure) {}

  Ttx::Concept::Interface::Relation relation;
  View::Bytes failure;
};

static auto evaluate(const Abstract& requirement, const Abstract& candidate)
    -> Evaluation {
  using Relation = Ttx::Concept::Interface::Relation;
  auto render = requirement.resolve().select<Render::Language::Structure>();
  auto shader = candidate.resolve().select<Shader::Language::Program>();
  if (!render || !shader) {
    return Evaluation(
        Relation::Rejected,
        "The restored relationship no longer selects Render and Shader owners."_view);
  }

  // Callable negotiation establishes shared value flow first. Render then adds
  // Stage and slot policy that Layout fitting deliberately omits.
  Ttx::Model::Interfaces::Callable callable_interface;
  for (const Reference<Abstract>& entry : render->get_callables()) {
    auto required = entry.get().select<Render::Language::Stage>();
    if (!required) {
      return Evaluation(
          Relation::Rejected,
          "The restored Render callable is not one Stage."_view);
    }
    auto supplied = find_function(*shader, required->get_name());
    if (!supplied) {
      return Evaluation(
          Relation::Rejected,
          "The restored Shader is missing one required Stage Function."_view);
    }
    if (!callable_interface.accepts(*required, *supplied)) {
      return Evaluation(
          Relation::Rejected,
          "The restored Stage Function no longer has a compatible Signature."_view);
    }
    if (!Render::Language::Attributes::satisfies(
            supplied->get_definition().get_attributes(),
            required->get_definition().get_attributes())) {
      return Evaluation(
          Relation::Rejected,
          "The restored Stage Function no longer satisfies its Render Attributes."_view);
    }
    View::Bytes parameter_failure = slot_failure(
        supplied->get_signature().get_parameters(),
        required->get_parameter_layout());
    if (!parameter_failure.is_empty()) {
      return Evaluation(Relation::Rejected, parameter_failure);
    }
    View::Bytes result_failure = slot_failure(
        supplied->get_signature().get_results(), required->get_result_layout());
    if (!result_failure.is_empty()) {
      return Evaluation(Relation::Rejected, result_failure);
    }
  }

  // Binding comparison preserves exact Type identity because matching storage
  // shape alone cannot establish one CPU and GPU relationship.
  for (const Reference<Abstract>& entry : render->get_addressables()) {
    auto required = entry.get().select<Render::Language::Binding>();
    if (!required) {
      return Evaluation(
          Relation::Rejected,
          "The restored Render value is not one Binding."_view);
    }
    auto supplied = find_binding(*shader, required->get_name());
    if (!supplied) {
      return Evaluation(
          Relation::Rejected,
          "The restored Shader is missing one required Binding."_view);
    }
    const Library::Language::Field& field = supplied->get_field();
    auto required_definition = required->get_definition();
    if (supplied->get_kind() != required->get_kind() ||
        &field.get_type() != &required->get_type()) {
      return Evaluation(
          Relation::Rejected,
          "The restored Shader Binding no longer has its required kind and Type."_view);
    }
    if (required_definition && !Render::Language::Attributes::satisfies(
                                   field.get_definition().get_attributes(),
                                   required_definition->get_attributes())) {
      return Evaluation(
          Relation::Rejected,
          "The restored Shader Binding no longer satisfies its Render Attributes."_view);
    }
  }
  return Evaluation(Relation::Satisfied);
}

auto Shader::Language::Contract::negotiate(
    const Abstract& requirement,
    const Abstract& candidate) const -> Relation {
  return evaluate(requirement, candidate).relation;
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

auto Shader::Language::Contract::validate_restored(
    const Abstract& requirement,
    const Abstract& candidate) const -> Bool {
  Evaluation evaluation = evaluate(requirement, candidate);
  if (evaluation.relation != Relation::Rejected) {
    return True;
  }
  Diagnostics::Log::error(evaluation.failure);
  return False;
}
