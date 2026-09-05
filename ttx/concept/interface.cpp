// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "ttx/concept/interface.hpp"

#include <cstdlib>

using namespace Ttx;

Interface::Interface(
    ttx_abstract requirement,
    ttx_abstract candidate,
    ttx_interface_relation relation)
    : binding(this),
      requirement(requirement),
      candidate(candidate),
      relation(relation) {}

auto Interface::get_abi() const -> ttx_interface {
  return &binding;
}

void Interface::publish(ttx_interface_sink result) const {
  result.operations->answer(result, get_abi());
}

void Interface::invoke(
    ttx_abstract,
    ttx_pack,
    ttx_context,
    ttx_pack_result result) const {
  result.operations->none(result);
}

auto Interface::select(ttx_interface self) -> const Interface& {
  if (self == nullptr || self->operations != &interface_operations) {
    std::abort();
  }
  const auto& selected = *static_cast<const Binding*>(self);
  if (selected.owner == nullptr || selected.owner->get_abi() != self) {
    std::abort();
  }
  return *selected.owner;
}

auto Interface::get_requirement_abi(ttx_interface self) -> ttx_abstract {
  return select(self).get_requirement();
}

auto Interface::get_candidate_abi(ttx_interface self) -> ttx_abstract {
  return select(self).get_candidate();
}

auto Interface::get_relation_abi(ttx_interface self) -> ttx_interface_relation {
  return select(self).get_relation();
}

void Interface::invoke_abi(
    ttx_interface self,
    ttx_abstract operation,
    ttx_pack input,
    ttx_context context,
    ttx_pack_result result) {
  const Interface& selected = select(self);
  if (selected.get_relation() == TTX_INTERFACE_UNKNOWN) {
    result.operations->unknown(result);
    return;
  }
  if (selected.get_relation() != TTX_INTERFACE_SATISFIED &&
      selected.get_relation() != TTX_INTERFACE_EQUIVALENT) {
    result.operations->none(result);
    return;
  }
  selected.invoke(operation, input, context, result);
}

const ttx_interface_ops Interface::interface_operations = {
  .header =
      {
        .size = sizeof(ttx_interface_ops),
        .abi_major = TTX_ABI_MAJOR,
        .abi_minor = TTX_ABI_MINOR,
      },
  .requirement = get_requirement_abi,
  .candidate = get_candidate_abi,
  .negotiate = get_relation_abi,
  .invoke = invoke_abi,
};
