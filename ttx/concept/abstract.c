// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "ttx/concept/abstract.h"

#include "ttx/concept/requirement_internal.h"

static const uint8_t name[] = "Abstract";

static const ttx_requirement requirement = {
    .abstract = {.operations = &ttx_requirement_operations},
    .name = {.data = name, .size = sizeof(name) - 1},
};

perimortem_bytes ttx_abstract_name(const ttx_abstract* abstract) {
  return abstract->operations->name(abstract);
}

const ttx_documentation* ttx_abstract_documentation(
    const ttx_abstract* abstract) {
  return abstract->operations->documentation(abstract);
}

const ttx_abstract* ttx_abstract_resolve(const ttx_abstract* abstract) {
  return abstract->operations->resolve(abstract);
}

const ttx_abstract* ttx_abstract_type(const ttx_abstract* abstract) {
  return abstract->operations->type(abstract);
}

const ttx_abstract* ttx_abstract_resolve_concept(
    const ttx_abstract* abstract,
    perimortem_bytes name) {
  return abstract->operations->resolve_concept(abstract, name);
}

void ttx_abstract_visit_concepts(
    const ttx_abstract* abstract,
    ttx_named_abstract_callable* visitor) {
  abstract->operations->visit_concepts(abstract, visitor);
}

ttx_interface ttx_abstract_interface(
    const ttx_abstract* abstract,
    const ttx_abstract* requirement) {
  return abstract->operations->interface(abstract, requirement);
}

const ttx_abstract* ttx_abstract_requirement(void) {
  return &requirement.abstract;
}
