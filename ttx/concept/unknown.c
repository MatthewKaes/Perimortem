// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "ttx/concept/unknown.h"

#include "ttx/concept/requirement_internal.h"

typedef struct ttx_unknown_value {
  ttx_abstract abstract;
  perimortem_bytes name;
} ttx_unknown_value;

static const uint8_t name[] = "Unknown";

static perimortem_bytes get_name(const ttx_abstract* self) {
  return ((const ttx_unknown_value*)self)->name;
}

static const ttx_documentation* documentation(const ttx_abstract* self) {
  (void)self;
  return ttx_documentation_empty();
}

static const ttx_abstract* identity(const ttx_abstract* self) {
  return self;
}

static const ttx_abstract* resolve_concept(
    const ttx_abstract* self,
    perimortem_bytes name) {
  (void)name;
  return self;
}

static void visit_concepts(
    const ttx_abstract* self,
    ttx_named_abstract_callable* visitor) {
  (void)self;
  (void)visitor;
}

static ttx_interface interface(
    const ttx_abstract* self,
    const ttx_abstract* requirement) {
  if (requirement == ttx_abstract_requirement() ||
      requirement == ttx_unknown_requirement()) {
    return ttx_interface_satisfied(
        requirement, self, ttx_interface_marker_operations());
  }
  return ttx_interface_rejected(requirement, self);
}

static const ttx_abstract_operations operations = {
    .name = get_name,
    .documentation = documentation,
    .resolve = identity,
    .type = identity,
    .resolve_concept = resolve_concept,
    .visit_concepts = visit_concepts,
    .interface = interface,
};

static const ttx_requirement requirement = {
    .abstract = {.operations = &ttx_requirement_operations},
    .name = {.data = name, .size = sizeof(name) - 1},
};

static const ttx_unknown_value value = {
    .abstract = {.operations = &operations},
    .name = {.data = name, .size = sizeof(name) - 1},
};

const ttx_abstract* ttx_unknown_requirement(void) {
  return &requirement.abstract;
}

const ttx_abstract* ttx_unknown(void) {
  return &value.abstract;
}

perimortem_bool ttx_unknown_prove(
    const ttx_abstract* candidate,
    ttx_unknown_view* view) {
  return ttx_requirement_prove(
      ttx_unknown_requirement(),
      candidate,
      &view->identity,
      &view->operations);
}
