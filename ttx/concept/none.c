// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "ttx/concept/none.h"

#include "ttx/concept/requirement_internal.h"
#include "ttx/concept/unknown.h"

struct ttx_none_value {
  struct ttx_abstract abstract;
  struct perimortem_bytes name;
};

static const uint8_t name[] = "None";
static const uint8_t fold_name[] = "fold";

static struct perimortem_bytes get_name(const struct ttx_abstract* self) {
  return ((const struct ttx_none_value*)self)->name;
}

static const struct ttx_documentation* documentation(
    const struct ttx_abstract* self) {
  (void)self;
  return ttx_documentation_empty();
}

static const struct ttx_abstract* identity(const struct ttx_abstract* self) {
  return self;
}

static const struct ttx_abstract* resolve_concept(
    const struct ttx_abstract* self,
    struct perimortem_bytes concept) {
  const struct perimortem_bytes fold = {
    .data = fold_name,
    .size = sizeof(fold_name) - 1,
  };
  return perimortem_bytes_equal(concept, fold) ? self : ttx_unknown();
}

static void visit_concepts(
    const struct ttx_abstract* self,
    struct ttx_named_abstract_callable* visitor) {
  const struct perimortem_bytes fold = {
    .data = fold_name,
    .size = sizeof(fold_name) - 1,
  };
  ttx_named_abstract_callable_call(visitor, fold, self);
}

static struct ttx_interface interface(
    const struct ttx_abstract* self,
    const struct ttx_abstract* requirement) {
  if (requirement == ttx_abstract_requirement() ||
      requirement == ttx_constant_requirement() ||
      requirement == ttx_none_requirement()) {
    return ttx_interface_satisfied(
        requirement, self, ttx_interface_marker_operations());
  }

  return ttx_interface_rejected(requirement, self);
}

static const struct ttx_abstract_operations operations = {
  .name = get_name,
  .documentation = documentation,
  .resolve = identity,
  .type = identity,
  .resolve_concept = resolve_concept,
  .visit_concepts = visit_concepts,
  .interface = interface,
};

static const struct ttx_requirement requirement = {
  .abstract = {.operations = &ttx_requirement_operations},
  .name = {.data = name, .size = sizeof(name) - 1},
};

static const struct ttx_none_value value = {
  .abstract = {.operations = &operations},
  .name = {.data = name, .size = sizeof(name) - 1},
};

const struct ttx_abstract* ttx_none_requirement(void) {
  return &requirement.abstract;
}

const struct ttx_abstract* ttx_none(void) {
  return &value.abstract;
}

perimortem_bool ttx_none_prove(
    const struct ttx_abstract* candidate,
    struct ttx_none_view* view) {
  return ttx_requirement_prove(
      ttx_none_requirement(), candidate, &view->identity, &view->operations);
}
