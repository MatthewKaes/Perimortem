// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "ttx/model/requirement_internal.h"

static struct perimortem_bytes name(const struct ttx_abstract* self) {
  return ((const struct ttx_model_requirement*)self)->name;
}

static const struct ttx_documentation* documentation(const struct ttx_abstract* self) {
  (void)self;
  return ttx_documentation_empty();
}

static const struct ttx_abstract* identity(const struct ttx_abstract* self) {
  return self;
}

static const struct ttx_abstract* type(const struct ttx_abstract* self) {
  (void)self;
  return ttx_none();
}

static const struct ttx_abstract* resolve_concept(
    const struct ttx_abstract* self,
    struct perimortem_bytes name) {
  (void)self;
  (void)name;
  return ttx_unknown();
}

static void visit_concepts(
    const struct ttx_abstract* self,
    struct ttx_named_abstract_callable* visitor) {
  (void)self;
  (void)visitor;
}

static struct ttx_interface interface(
    const struct ttx_abstract* self,
    const struct ttx_abstract* requirement) {
  return requirement == ttx_abstract_requirement()
             ? ttx_interface_satisfied(
                   requirement,
                   self,
                   ttx_interface_marker_operations())
             : ttx_interface_rejected(requirement, self);
}

const struct ttx_abstract_operations ttx_model_requirement_operations = {
    .name = name,
    .documentation = documentation,
    .resolve = identity,
    .type = type,
    .resolve_concept = resolve_concept,
    .visit_concepts = visit_concepts,
    .interface = interface,
};
