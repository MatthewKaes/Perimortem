// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "ttx/model/alias.hpp"

#include <cstdlib>

#include "ttx/query.hpp"

using namespace Ttx;

Alias::Alias() : target(ttx_unknown()) {}

Alias::Alias(ttx_abstract target) : Alias() {
  if (!bind(target)) {
    std::abort();
  }
}

auto Alias::bind(ttx_abstract candidate) -> bool {
  // Self reference is visible only before spot resolution because an unbound
  // Alias truthfully resolves to Unknown. Rejecting it here keeps that honest
  // provisional answer from concealing a cycle in the authored edge.
  if (ttx_abstract_same(candidate, get_abi())) {
    return false;
  }
  const ttx_abstract selected = Ttx::resolve(candidate);
  if (ttx_abstract_same(selected, ttx_none()) ||
      ttx_abstract_same(selected, get_abi())) {
    return false;
  }
  if (ttx_abstract_same(target, ttx_unknown())) {
    target = selected;
    return true;
  }
  return ttx_abstract_same(target, selected);
}

auto Alias::name() const -> ttx_borrowed_bytes {
  return target->operations->name(target);
}

auto Alias::documentation(ttx_abstract) const -> ttx_documentation {
  return target->operations->documentation(target);
}

auto Alias::resolve(ttx_abstract) const -> ttx_abstract {
  return target;
}

auto Alias::resolve_concept(ttx_borrowed_bytes route) const -> ttx_abstract {
  return Ttx::resolve_concept(target, route);
}

void Alias::visit_concepts(ttx_concept_sink result) const {
  target->operations->visit_concepts(target, result);
}

void Alias::interface(
    ttx_abstract,
    ttx_abstract requirement,
    ttx_interface_sink result) const {
  target->operations->interface(target, requirement, result);
}

void Alias::domain(ttx_abstract, ttx_domain_result result) const {
  target->operations->resolve_domain(target, result);
}

void Alias::callable(ttx_abstract, ttx_callable_result result) const {
  target->operations->resolve_callable(target, result);
}

void Alias::route(ttx_abstract, ttx_route_result result) const {
  target->operations->resolve_route(target, result);
}

void Alias::finite_extent(ttx_abstract, ttx_finite_extent_result result) const {
  target->operations->resolve_finite_extent(target, result);
}

void Alias::bytes(ttx_abstract, ttx_bytes_result result) const {
  target->operations->resolve_bytes(target, result);
}
