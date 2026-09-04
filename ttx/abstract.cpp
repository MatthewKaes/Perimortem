// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "ttx/abstract.hpp"

#include <cstddef>
#include <cstdlib>

using namespace Ttx;

Abstract::Abstract() : Abstract(ttx_authority_create(), 1) {}

Abstract::Abstract(uint64_t authority, uint64_t value)
    : binding({
        .operations =
            {
              .header =
                  {
                    .size = sizeof(ttx_abstract_ops),
                    .abi_major = TTX_ABI_MAJOR,
                    .abi_minor = TTX_ABI_MINOR,
                  },
              .name = get_name,
              .documentation = get_documentation,
              .resolve = resolve_abi,
              .resolve_concept = resolve_concept_abi,
              .visit_concepts = visit_concepts_abi,
              .interface = interface_abi,
              .resolve_domain = resolve_domain_abi,
              .resolve_callable = resolve_callable_abi,
              .resolve_route = resolve_route_abi,
              .resolve_finite_extent = resolve_finite_extent_abi,
            },
        .owner = this,
        .authority = authority,
        .value = value,
      }) {
  if (authority == 0 || value == 0) {
    std::abort();
  }
}

auto Abstract::get_abi() const -> ttx_abstract {
  return {
    .operations = &binding.operations,
    .owner = binding.authority,
    .value = binding.value,
  };
}

auto Abstract::documentation(ttx_abstract) const -> ttx_documentation {
  const ttx_abstract none = ttx_none();
  return none.operations->documentation(none);
}

auto Abstract::resolve(ttx_abstract self) const -> ttx_abstract {
  return self;
}

auto Abstract::resolve_concept(ttx_borrowed_bytes) const -> ttx_abstract {
  return ttx_unknown();
}

void Abstract::visit_concepts(ttx_concept_sink result) const {
  result.operations->completed(result);
}

auto Abstract::negotiate(ttx_abstract requirement) const
    -> ttx_interface_relation {
  return ttx_abstract_same(get_abi(), requirement) ? TTX_INTERFACE_EQUIVALENT
                                                   : TTX_INTERFACE_REJECTED;
}

void Abstract::interface(
    ttx_abstract self,
    ttx_abstract requirement,
    ttx_interface_sink result) const {
  const InterfaceBinding answer = {
    .operations =
        {
          .header =
              {
                .size = sizeof(ttx_interface_ops),
                .abi_major = TTX_ABI_MAJOR,
                .abi_minor = TTX_ABI_MINOR,
              },
          .requirement = interface_requirement,
          .candidate = interface_candidate,
          .negotiate = interface_negotiate,
          .invoke = interface_invoke,
        },
    .owner = this,
    .requirement = requirement,
    .candidate = self,
    .relation = negotiate(requirement),
  };
  const ttx_interface selected = {
    .operations = &answer.operations,
    .owner = self.owner,
    .value = self.value,
  };
  result.operations->answer(result, selected);
}

void Abstract::domain(ttx_abstract, ttx_domain_result result) const {
  result.operations->none(result);
}

void Abstract::callable(ttx_abstract, ttx_callable_result result) const {
  result.operations->none(result);
}

void Abstract::route(ttx_abstract, ttx_route_result result) const {
  result.operations->none(result);
}

void Abstract::finite_extent(ttx_abstract, ttx_finite_extent_result result)
    const {
  result.operations->none(result);
}

void Abstract::invoke(
    ttx_abstract,
    ttx_pack,
    ttx_context,
    ttx_pack_result result) const {
  result.operations->none(result);
}

auto Abstract::select(ttx_abstract self) -> Abstract& {
  if (self.operations == nullptr) {
    std::abort();
  }

  // Binding begins at the operations table so native dispatch can recover its
  // owner without putting a process address into the semantic identity. A
  // foreign bridge may use different private machinery behind the same tokens.
  static_assert(offsetof(Binding, operations) == 0);
  const auto& selected = *reinterpret_cast<const Binding*>(self.operations);
  if (selected.owner == nullptr || selected.authority != self.owner ||
      selected.value != self.value) {
    std::abort();
  }
  return *selected.owner;
}

auto Abstract::select(ttx_interface self) -> const InterfaceBinding& {
  if (self.operations == nullptr) {
    std::abort();
  }

  // Interface answers borrow one callback frame. Its table remains a private
  // dispatch anchor while the sink observes the immutable relationship.
  static_assert(offsetof(InterfaceBinding, operations) == 0);
  const auto& selected =
      *reinterpret_cast<const InterfaceBinding*>(self.operations);
  if (selected.owner == nullptr || selected.candidate.owner != self.owner ||
      selected.candidate.value != self.value) {
    std::abort();
  }
  return selected;
}

auto TTX_CALL Abstract::get_name(ttx_abstract self) -> ttx_borrowed_bytes {
  return select(self).name();
}

auto TTX_CALL Abstract::get_documentation(ttx_abstract self)
    -> ttx_documentation {
  return select(self).documentation(self);
}

void TTX_CALL
    Abstract::resolve_abi(ttx_abstract self, ttx_abstract_sink result) {
  result.operations->answer(result, select(self).resolve(self));
}

void TTX_CALL Abstract::resolve_concept_abi(
    ttx_abstract self,
    ttx_borrowed_bytes route,
    ttx_abstract_sink result) {
  result.operations->answer(result, select(self).resolve_concept(route));
}

void TTX_CALL
    Abstract::visit_concepts_abi(ttx_abstract self, ttx_concept_sink result) {
  select(self).visit_concepts(result);
}

void TTX_CALL Abstract::interface_abi(
    ttx_abstract self,
    ttx_abstract requirement,
    ttx_interface_sink result) {
  select(self).interface(self, requirement, result);
}

void TTX_CALL
    Abstract::resolve_domain_abi(ttx_abstract self, ttx_domain_result result) {
  select(self).domain(self, result);
}

void TTX_CALL Abstract::resolve_callable_abi(
    ttx_abstract self,
    ttx_callable_result result) {
  select(self).callable(self, result);
}

void TTX_CALL
    Abstract::resolve_route_abi(ttx_abstract self, ttx_route_result result) {
  select(self).route(self, result);
}

void TTX_CALL Abstract::resolve_finite_extent_abi(
    ttx_abstract self,
    ttx_finite_extent_result result) {
  select(self).finite_extent(self, result);
}

auto TTX_CALL Abstract::interface_requirement(ttx_interface self)
    -> ttx_abstract {
  return select(self).requirement;
}

auto TTX_CALL Abstract::interface_candidate(ttx_interface self)
    -> ttx_abstract {
  return select(self).candidate;
}

auto TTX_CALL Abstract::interface_negotiate(ttx_interface self)
    -> ttx_interface_relation {
  return select(self).relation;
}

void TTX_CALL Abstract::interface_invoke(
    ttx_interface self,
    ttx_abstract operation,
    ttx_pack input,
    ttx_context context,
    ttx_pack_result result) {
  const InterfaceBinding& selected = select(self);
  if (selected.relation != TTX_INTERFACE_SATISFIED &&
      selected.relation != TTX_INTERFACE_EQUIVALENT) {
    result.operations->none(result);
    return;
  }
  selected.owner->invoke(operation, input, context, result);
}
