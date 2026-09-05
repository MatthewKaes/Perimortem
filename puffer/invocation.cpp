// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "puffer/invocation.hpp"

#include <cstddef>

#include "ttx/model/requirement.hpp"

using namespace Perimortem::Core;
using namespace Puffer;

static auto bytes_view(const std::vector<uint8_t>& value) -> View::Bytes {
  return value.empty() ? View::Bytes()
                       : View::Bytes(value.data(), value.size());
}

auto Invocation::copy(View::Bytes value) -> std::vector<uint8_t> {
  return value.is_empty()
             ? std::vector<uint8_t>()
             : std::vector<uint8_t>(
                   value.get_data(), value.get_data() + value.get_size());
}

Invocation::Input::Input(View::Bytes input_name, View::Bytes input_value)
    : name_storage(copy(input_name)),
      value_storage(copy(input_value)),
      name(bytes_view(name_storage)),
      value(bytes_view(value_storage)),
      binding({
        .operations =
            {
              .header =
                  {
                    .size = sizeof(ttx_bytes_ops),
                    .abi_major = TTX_ABI_MAJOR,
                    .abi_minor = TTX_ABI_MINOR,
                  },
              .candidate = candidate,
              .size = size,
              .visit = visit,
            },
      }) {}

auto Invocation::Input::select(ttx_bytes self) -> const Input& {
  return *reinterpret_cast<const Input*>(self.self);
}

auto Invocation::Input::candidate(ttx_bytes self) -> ttx_abstract {
  return select(self).get_handle();
}

auto Invocation::Input::size(ttx_bytes self) -> uint64_t {
  return select(self).value.get_size();
}

void Invocation::Input::visit(ttx_bytes self, ttx_bytes_sink sink) {
  const View::Bytes value = select(self).value;
  if (!value.is_empty()) {
    sink.operations->bytes(
        sink, {.data = value.get_data(), .size = value.get_size()});
  }
  sink.operations->completed(sink);
}

void Invocation::Input::bytes(ttx_abstract self, ttx_bytes_result result)
    const {
  (void)self;
  result.operations->resolved(
      result, {
                .operations = &binding.operations,
                .self = reinterpret_cast<ttx_bytes_self*>(
                    const_cast<Input*>(this)),
              });
}

auto Invocation::Input::negotiate(ttx_abstract requirement) const
    -> ttx_interface_relation {
  return ttx_abstract_same(requirement, ttx_bytes_requirement()) ||
                 ttx_abstract_same(requirement, ttx_constant_requirement())
             ? TTX_INTERFACE_SATISFIED
             : Ttx::Model::Domain::negotiate(requirement);
}

Invocation::Inputs::Inputs(
    const std::vector<const Ttx::Concept::Abstract*>& entries)
    : layout(
          View::Vector<const Ttx::Concept::Abstract*>(
              entries.data(),
              entries.size())) {}

auto Invocation::Inputs::get_layout() const -> const Ttx::Concept::Layout& {
  return layout;
}

Invocation::Invocation(
    View::Bytes selected_working_directory,
    View::Bytes selected_sdk,
    const std::vector<View::Bytes>& selected_arguments)
    : argument_values(),
      argument_entries(),
      working_directory("working directory"_view, selected_working_directory),
      sdk("SDK"_view, selected_sdk),
      arguments() {
  argument_values.reserve(selected_arguments.size());
  argument_entries.reserve(selected_arguments.size());
  for (View::Bytes value : selected_arguments) {
    argument_values.push_back(std::make_unique<Input>("argument"_view, value));
    argument_entries.push_back(argument_values.back().get());
  }

  // Inputs borrows the completed vector storage. Constructing it after the
  // final insertion prevents a later vector growth from moving the addresses
  // used by its Layout.
  arguments = std::make_unique<Inputs>(argument_entries);
}

auto Invocation::resolve_concept(View::Bytes route) const
    -> const Ttx::Concept::Abstract& {
  if (route == "arguments"_view) {
    return *arguments;
  }
  if (route == "working_directory"_view) {
    return working_directory;
  }
  if (route == "sdk"_view) {
    return sdk;
  }
  return Ttx::Concept::Abstract::resolve_concept(route);
}

void Invocation::visit_concepts(ttx_named_abstract_callable* visitor) const {
  visit_concept(visitor, "arguments"_view, *arguments);
  visit_concept(visitor, "working_directory"_view, working_directory);
  visit_concept(visitor, "sdk"_view, sdk);
}
