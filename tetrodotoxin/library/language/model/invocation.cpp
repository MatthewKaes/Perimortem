// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/library/language/model/invocation.hpp"

#include <cstddef>

#include "ttx/query.hpp"

using namespace Perimortem::Core;
using namespace Tetrodotoxin::Library::Language;

struct InputCapture {
  ttx_layout_entry_sink_ops operations;
  std::vector<const Model::Pack*>* inputs;
  bool complete;
  bool valid;
};

struct EnumerableCapture {
  ttx_enumerable_result_ops operations;
  bool answered;
  ttx_enumerable enumerable;
};

template <typename Owner, typename Self>
static auto select_owner(Self* self) -> Owner& {
  return *reinterpret_cast<Owner*>(self);
}

static void TTX_CALL enumerable_rejected(ttx_enumerable_result self) {
  auto& capture = select_owner<EnumerableCapture>(self.self);
  capture.answered = true;
  capture.enumerable = {};
}

static void TTX_CALL enumerable_satisfied(
    ttx_enumerable_result self,
    ttx_enumerable enumerable) {
  auto& capture = select_owner<EnumerableCapture>(self.self);
  capture.answered = true;
  capture.enumerable = enumerable;
}

static void TTX_CALL input(
    ttx_layout_entry_sink self,
    ttx_borrowed_bytes,
    ttx_abstract producer) {
  auto& capture = select_owner<InputCapture>(self.self);
  auto owner = Ttx::Concept::Abstract::from_handle(producer);
  auto pack =
      owner ? Model::Pack::from(const_cast<Ttx::Concept::Abstract&>(*owner))
            : Option<Model::Pack&>();
  if (!pack) {
    capture.valid = false;
    return;
  }
  capture.inputs->push_back(&*pack);
}

static void TTX_CALL inputs_completed(ttx_layout_entry_sink self) {
  select_owner<InputCapture>(self.self).complete = true;
}

auto Model::Invocation::requirement() -> ttx_abstract {
  static constinit Ttx::Requirement requirement(
      "Tetrodotoxin.Library.Invocation"_bytes);
  return requirement.get_abi();
}

auto Model::Invocation::operation() -> ttx_abstract {
  static constinit Ttx::Requirement operation(
      "Tetrodotoxin.Library.Invoke"_bytes);
  return operation.get_abi();
}

auto Model::Invocation::call(
    ttx_abstract callable,
    ttx_pack input,
    ttx_context context) -> Ttx::PackObservation {
  return Ttx::invoke(callable, requirement(), operation(), input, context);
}

auto Model::Invocation::local_inputs(ttx_pack pack)
    -> Option<std::vector<const Model::Pack*>> {
  if (pack.operations == nullptr) {
    return {};
  }
  const ttx_layout layout = pack.operations->layout(pack);
  EnumerableCapture enumerable = {
    .operations =
        {
          .header =
              {
                .size = sizeof(ttx_enumerable_result_ops),
                .abi_major = TTX_ABI_MAJOR,
                .abi_minor = TTX_ABI_MINOR,
              },
          .rejected = enumerable_rejected,
          .satisfied = enumerable_satisfied,
        },
    .answered = false,
    .enumerable = {},
  };
  const ttx_enumerable_result result = {
    .operations = &enumerable.operations,
    .self = reinterpret_cast<ttx_enumerable_result_self*>(&enumerable),
  };
  layout.operations->enumerable(layout, result);
  if (!enumerable.answered || enumerable.enumerable.operations == nullptr) {
    return {};
  }
  std::vector<const Model::Pack*> inputs;
  InputCapture capture = {
    .operations =
        {
          .header =
              {
                .size = sizeof(ttx_layout_entry_sink_ops),
                .abi_major = TTX_ABI_MAJOR,
                .abi_minor = TTX_ABI_MINOR,
              },
          .entry = input,
          .completed = inputs_completed,
        },
    .inputs = &inputs,
    .complete = false,
    .valid = true,
  };
  const ttx_layout_entry_sink sink = {
    .operations = &capture.operations,
    .self = reinterpret_cast<ttx_layout_entry_sink_self*>(&capture),
  };
  enumerable.enumerable.operations->visit(enumerable.enumerable, sink);
  if (!capture.complete || !capture.valid ||
      inputs.size() != enumerable.enumerable.operations->cardinality(
                           enumerable.enumerable)) {
    return {};
  }
  return inputs;
}

void Model::Invocation::return_pack(
    Option<Model::Pack&> pack,
    ttx_context context,
    ttx_pack_result result) {
  if (!pack) {
    result.operations->none(result);
    return;
  }
  const Ttx::PackObservation projected = pack->retain(context);
  switch (projected.state) {
  case Ttx::PackObservationState::Unknown:
    result.operations->unknown(result);
    return;
  case Ttx::PackObservationState::None:
    result.operations->none(result);
    return;
  case Ttx::PackObservationState::Packed:
    result.operations->packed(result, projected.pack);
    return;
  case Ttx::PackObservationState::SupportFailed:
    result.operations->support_failed(result, projected.failure);
    return;
  }
}
