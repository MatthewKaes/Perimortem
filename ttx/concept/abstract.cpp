// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "ttx/concept/abstract.hpp"

#include <cstddef>
#include <cstdlib>

#include "ttx/concept/interface.hpp"
#include "ttx/concept/none.hpp"
#include "ttx/concept/unknown.hpp"
#include "ttx/query.hpp"

using namespace Ttx;
using namespace Perimortem::Core;

auto Abstract::get_abi() const -> ttx_abstract {
  return &binding;
}

auto Abstract::name() const -> ttx_borrowed_bytes {
  const View::Bytes value = get_name();
  return {
    .data = value.get_data(),
    .size = value.get_size(),
  };
}

auto Abstract::documentation(ttx_abstract) const -> ttx_documentation {
  return get_documentation().get_abi();
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
  const Interface answer(requirement, self, negotiate(requirement));
  answer.publish(result);
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

void Abstract::bytes(ttx_abstract, ttx_bytes_result result) const {
  result.operations->none(result);
}

auto Abstract::dispatch(ttx_abstract self) -> const Abstract& {
  if (self == nullptr || self->operations != &abstract_operations) {
    std::abort();
  }
  const auto& selected = *static_cast<const AbiBinding*>(self);
  if (selected.owner == nullptr || selected.owner->get_abi() != self) {
    std::abort();
  }
  return *selected.owner;
}

const ttx_abstract_ops Abstract::abstract_operations = {
  .header =
      {
        .size = sizeof(ttx_abstract_ops),
        .abi_major = TTX_ABI_MAJOR,
        .abi_minor = TTX_ABI_MINOR,
      },
  .name = get_name_abi,
  .documentation = get_documentation_abi,
  .resolve = resolve_abi,
  .resolve_concept = resolve_concept_abi,
  .visit_concepts = visit_concepts_abi,
  .interface = interface_abi,
  .resolve_domain = resolve_domain_abi,
  .resolve_callable = resolve_callable_abi,
  .resolve_route = resolve_route_abi,
  .resolve_finite_extent = resolve_finite_extent_abi,
  .resolve_bytes = resolve_bytes_abi,
};

auto TTX_CALL Abstract::get_name_abi(ttx_abstract self) -> ttx_borrowed_bytes {
  return dispatch(self).name();
}

auto TTX_CALL Abstract::get_documentation_abi(ttx_abstract self)
    -> ttx_documentation {
  return dispatch(self).documentation(self);
}

void TTX_CALL
    Abstract::resolve_abi(ttx_abstract self, ttx_abstract_sink result) {
  result.operations->answer(result, dispatch(self).resolve(self));
}

void TTX_CALL Abstract::resolve_concept_abi(
    ttx_abstract self,
    ttx_borrowed_bytes route,
    ttx_abstract_sink result) {
  result.operations->answer(result, dispatch(self).resolve_concept(route));
}

void TTX_CALL
    Abstract::visit_concepts_abi(ttx_abstract self, ttx_concept_sink result) {
  dispatch(self).visit_concepts(result);
}

void TTX_CALL Abstract::interface_abi(
    ttx_abstract self,
    ttx_abstract requirement,
    ttx_interface_sink result) {
  dispatch(self).interface(self, requirement, result);
}

void TTX_CALL
    Abstract::resolve_domain_abi(ttx_abstract self, ttx_domain_result result) {
  dispatch(self).domain(self, result);
}

void TTX_CALL Abstract::resolve_callable_abi(
    ttx_abstract self,
    ttx_callable_result result) {
  dispatch(self).callable(self, result);
}

void TTX_CALL
    Abstract::resolve_route_abi(ttx_abstract self, ttx_route_result result) {
  dispatch(self).route(self, result);
}

void TTX_CALL Abstract::resolve_finite_extent_abi(
    ttx_abstract self,
    ttx_finite_extent_result result) {
  dispatch(self).finite_extent(self, result);
}

void TTX_CALL
    Abstract::resolve_bytes_abi(ttx_abstract self, ttx_bytes_result result) {
  dispatch(self).bytes(self, result);
}
