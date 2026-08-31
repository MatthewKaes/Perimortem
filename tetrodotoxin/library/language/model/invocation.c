// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/library/language/model/invocation.h"

struct invocation_requirement {
  struct ttx_abstract abstract;
};

static const uint8_t invocation_name[] = "Invocation";

static struct perimortem_bytes requirement_name(
    const struct ttx_abstract* self) {
  const struct perimortem_bytes name = {
    .data = invocation_name,
    .size = sizeof(invocation_name) - 1,
  };
  (void)self;
  return name;
}

static const struct ttx_documentation* requirement_documentation(
    const struct ttx_abstract* self) {
  (void)self;
  return ttx_documentation_empty();
}

static const struct ttx_abstract* requirement_identity(
    const struct ttx_abstract* self) {
  return self;
}

static const struct ttx_abstract* requirement_type(
    const struct ttx_abstract* self) {
  (void)self;
  return ttx_none();
}

static const struct ttx_abstract* requirement_concept(
    const struct ttx_abstract* self,
    struct perimortem_bytes name) {
  (void)self;
  (void)name;
  return ttx_unknown();
}

static void requirement_concepts(
    const struct ttx_abstract* self,
    struct ttx_named_abstract_callable* visitor) {
  (void)self;
  (void)visitor;
}

static struct ttx_interface requirement_interface(
    const struct ttx_abstract* self,
    const struct ttx_abstract* requirement) {
  return ttx_interface_rejected(requirement, self);
}

static const struct ttx_abstract_operations requirement_operations = {
  .name = requirement_name,
  .documentation = requirement_documentation,
  .resolve = requirement_identity,
  .type = requirement_type,
  .resolve_concept = requirement_concept,
  .visit_concepts = requirement_concepts,
  .interface = requirement_interface,
};

static const struct invocation_requirement requirement_value = {
  .abstract = {.operations = &requirement_operations},
};

const struct ttx_abstract* ttx_library_invocation_requirement(void) {
  return &requirement_value.abstract;
}

perimortem_bool ttx_library_invocation_prove(
    const struct ttx_abstract* candidate,
    struct ttx_library_invocation_view* view) {
  const struct ttx_interface relation =
      ttx_abstract_interface(candidate, ttx_library_invocation_requirement());
  if (!ttx_interface_accepts(&relation)) {
    return PERIMORTEM_FALSE;
  }

  view->identity = candidate;
  view->operations =
      (const struct ttx_library_invocation_operations*)relation.operations;
  return PERIMORTEM_TRUE;
}

const struct ttx_pack* ttx_library_invoke(
    const struct ttx_library_invocation_view* callable,
    const struct ttx_pack* receiver,
    const struct ttx_pack* arguments) {
  return callable->operations->invoke(callable->identity, receiver, arguments);
}

ttx_interface_relation ttx_library_invocation_relation(
    const struct ttx_abstract* requirement,
    const struct ttx_abstract* candidate) {
  (void)requirement;
  (void)candidate;
  return TTX_INTERFACE_SATISFIED;
}
