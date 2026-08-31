// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "ttx/concept/abstract.h"

#include "ttx/concept/requirement_internal.h"

static const uint8_t name[] = "Abstract";

static const struct ttx_requirement requirement = {
  .abstract = {.operations = &ttx_requirement_operations},
  .name = {.data = name, .size = sizeof(name) - 1},
};

struct perimortem_bytes ttx_abstract_name(const struct ttx_abstract* abstract) {
  return abstract->operations->name(abstract);
}

const struct ttx_documentation* ttx_abstract_documentation(
    const struct ttx_abstract* abstract) {
  return abstract->operations->documentation(abstract);
}

const struct ttx_abstract* ttx_abstract_resolve(
    const struct ttx_abstract* abstract) {
  return abstract->operations->resolve(abstract);
}

const struct ttx_abstract* ttx_abstract_type(
    const struct ttx_abstract* abstract) {
  return abstract->operations->type(abstract);
}

const struct ttx_abstract* ttx_abstract_resolve_concept(
    const struct ttx_abstract* abstract,
    struct perimortem_bytes name) {
  return abstract->operations->resolve_concept(abstract, name);
}

void ttx_abstract_visit_concepts(
    const struct ttx_abstract* abstract,
    struct ttx_named_abstract_callable* visitor) {
  abstract->operations->visit_concepts(abstract, visitor);
}

struct ttx_interface ttx_abstract_interface(
    const struct ttx_abstract* abstract,
    const struct ttx_abstract* requirement) {
  return abstract->operations->interface(abstract, requirement);
}

const struct ttx_abstract* ttx_abstract_requirement(void) {
  return &requirement.abstract;
}
