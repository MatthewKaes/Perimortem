// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "ttx/model/requirement_internal.h"

static perimortem_bytes name(const ttx_abstract* self) {
  return ((const ttx_model_requirement*)self)->name;
}

static const ttx_documentation* documentation(const ttx_abstract* self) {
  (void)self;
  return ttx_documentation_empty();
}

static const ttx_abstract* identity(const ttx_abstract* self) {
  return self;
}

static const ttx_abstract* type(const ttx_abstract* self) {
  (void)self;
  return ttx_none();
}

static const ttx_abstract* resolve_concept(
    const ttx_abstract* self,
    perimortem_bytes name) {
  (void)self;
  (void)name;
  return ttx_unknown();
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
  return requirement == ttx_abstract_requirement()
             ? ttx_interface_satisfied(
                   requirement,
                   self,
                   ttx_interface_marker_operations())
             : ttx_interface_rejected(requirement, self);
}

const ttx_abstract_operations ttx_model_requirement_operations = {
    .name = name,
    .documentation = documentation,
    .resolve = identity,
    .type = type,
    .resolve_concept = resolve_concept,
    .visit_concepts = visit_concepts,
    .interface = interface,
};
