// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/library/language/model/fold_call.h"

typedef struct fold_call_requirement {
  ttx_abstract abstract;
} fold_call_requirement;

static const uint8_t fold_call_name[] = "FoldCall";

static perimortem_bytes requirement_name(const ttx_abstract* self) {
  perimortem_bytes name = {
      .data = fold_call_name,
      .size = sizeof(fold_call_name) - 1,
  };
  (void)self;
  return name;
}

static const ttx_documentation* requirement_documentation(
    const ttx_abstract* self) {
  (void)self;
  return ttx_documentation_empty();
}

static const ttx_abstract* requirement_identity(const ttx_abstract* self) {
  return self;
}

static const ttx_abstract* requirement_type(const ttx_abstract* self) {
  (void)self;
  return ttx_none();
}

static const ttx_abstract* requirement_concept(
    const ttx_abstract* self,
    perimortem_bytes name) {
  (void)self;
  (void)name;
  return ttx_unknown();
}

static void requirement_concepts(
    const ttx_abstract* self,
    ttx_named_abstract_callable* visitor) {
  (void)self;
  (void)visitor;
}

static ttx_interface requirement_interface(
    const ttx_abstract* self,
    const ttx_abstract* requirement) {
  return ttx_interface_rejected(requirement, self);
}

static const ttx_abstract_operations requirement_operations = {
    .name = requirement_name,
    .documentation = requirement_documentation,
    .resolve = requirement_identity,
    .type = requirement_type,
    .resolve_concept = requirement_concept,
    .visit_concepts = requirement_concepts,
    .interface = requirement_interface,
};

static const fold_call_requirement requirement_value = {
    .abstract = {.operations = &requirement_operations},
};

const ttx_abstract* ttx_library_fold_call_requirement(void) {
  return &requirement_value.abstract;
}

perimortem_bool ttx_library_fold_call_prove(
    const ttx_abstract* candidate,
    ttx_library_fold_call_view* view) {
  ttx_interface relation = ttx_abstract_interface(
      candidate, ttx_library_fold_call_requirement());
  if (!ttx_interface_accepts(&relation)) {
    return PERIMORTEM_FALSE;
  }
  view->identity = candidate;
  view->operations =
      (const ttx_library_fold_call_operations*)relation.operations;
  return PERIMORTEM_TRUE;
}

const ttx_abstract* ttx_library_fold_call(
    const ttx_library_fold_call_view* callable,
    const ttx_pack* receiver,
    const ttx_pack* arguments) {
  return callable->operations->fold(
      callable->identity, receiver, arguments);
}

ttx_interface_relation ttx_library_fold_call_relation(
    const ttx_abstract* requirement,
    const ttx_abstract* candidate) {
  (void)requirement;
  (void)candidate;
  return TTX_INTERFACE_SATISFIED;
}
