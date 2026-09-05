// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "cross_language/bridge.h"

static uint8_t route_is(ttx_borrowed_bytes route, const char* text) {
  uint64_t size = 0;
  while (text[size] != '\0') {
    ++size;
  }
  if (route.size != size || (size != 0 && route.data == 0)) {
    return 0;
  }
  for (uint64_t index = 0; index < size; ++index) {
    if (route.data[index] != (uint8_t)text[index]) {
      return 0;
    }
  }
  return 1;
}

static void TTX_CALL resolve_concept(
    ttx_addressable_policy self,
    ttx_abstract candidate,
    ttx_borrowed_bytes route,
    ttx_abstract_sink result) {
  (void)self;
  (void)candidate;
  result.operations->answer(
      result, route_is(route, "private") ? ttx_unknown() : ttx_none());
}

static void TTX_CALL visit_concepts(
    ttx_addressable_policy self,
    ttx_abstract candidate,
    ttx_concept_sink result) {
  (void)self;
  (void)candidate;
  result.operations->completed(result);
}

static void TTX_CALL interface_policy(
    ttx_addressable_policy self,
    ttx_abstract candidate,
    ttx_abstract requirement,
    ttx_addressable_interface_result result) {
  (void)self;
  (void)candidate;
  (void)requirement;
  result.operations->pass(result);
}

static void TTX_CALL domain_policy(
    ttx_addressable_policy self,
    ttx_abstract candidate,
    ttx_domain_result result) {
  (void)self;
  (void)candidate;
  result.operations->none(result);
}

static void TTX_CALL callable_policy(
    ttx_addressable_policy self,
    ttx_abstract candidate,
    ttx_callable_result result) {
  (void)self;
  (void)candidate;
  result.operations->none(result);
}

static void TTX_CALL route_policy(
    ttx_addressable_policy self,
    ttx_abstract candidate,
    ttx_route_result result) {
  (void)self;
  (void)candidate;
  result.operations->none(result);
}

static void TTX_CALL extent_policy(
    ttx_addressable_policy self,
    ttx_abstract candidate,
    ttx_finite_extent_result result) {
  (void)self;
  (void)candidate;
  result.operations->none(result);
}

static void TTX_CALL bytes_policy(
    ttx_addressable_policy self,
    ttx_abstract candidate,
    ttx_bytes_result result) {
  (void)self;
  (void)candidate;
  result.operations->none(result);
}

static void TTX_CALL invoke_policy(
    ttx_addressable_policy self,
    ttx_abstract candidate,
    ttx_abstract requirement,
    ttx_abstract operation,
    ttx_pack input,
    ttx_context context,
    ttx_pack_result result) {
  (void)self;
  (void)candidate;
  (void)requirement;
  (void)operation;
  (void)input;
  (void)context;
  result.operations->none(result);
}

static const ttx_addressable_policy_ops policy_operations = {
  .header =
      {
        .size = sizeof(ttx_addressable_policy_ops),
        .abi_major = TTX_ABI_MAJOR,
        .abi_minor = TTX_ABI_MINOR,
      },
  .resolve_concept = resolve_concept,
  .visit_concepts = visit_concepts,
  .interface = interface_policy,
  .resolve_domain = domain_policy,
  .resolve_callable = callable_policy,
  .resolve_route = route_policy,
  .resolve_finite_extent = extent_policy,
  .resolve_bytes = bytes_policy,
  .invoke = invoke_policy,
};

ttx_addressable_policy TTX_CALL ttx_test_c_restriction_policy(void) {
  static uint8_t policy_state;
  const ttx_addressable_policy policy = {
    .operations = &policy_operations,
    .self = (ttx_addressable_policy_self*)&policy_state,
  };
  return policy;
}
