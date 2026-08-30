// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "ttx/model/type.h"

#include "ttx/model/requirement_internal.h"

static const uint8_t name[] = "Type";

static const struct ttx_model_requirement requirement = {
    .abstract = {.operations = &ttx_model_requirement_operations},
    .name = {.data = name, .size = sizeof(name) - 1},
};

const struct ttx_abstract* ttx_type_requirement(void) {
  return &requirement.abstract;
}

perimortem_bool ttx_type_prove(
    const struct ttx_abstract* candidate,
    struct ttx_type_view* view) {
  struct ttx_interface interface =
      ttx_abstract_interface(candidate, ttx_type_requirement());
  if (!ttx_interface_accepts(&interface)) {
    return PERIMORTEM_FALSE;
  }
  view->identity = candidate;
  view->operations = (const struct ttx_type_operations*)interface.operations;
  return PERIMORTEM_TRUE;
}

const struct ttx_layout* ttx_type_layout(const struct ttx_type_view* type) {
  return type->operations->layout(type->identity);
}
