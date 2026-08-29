// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "ttx/model/type.h"

#include "ttx/model/requirement_internal.h"

static const uint8_t name[] = "Type";

static const ttx_model_requirement requirement = {
    .abstract = {.operations = &ttx_model_requirement_operations},
    .name = {.data = name, .size = sizeof(name) - 1},
};

const ttx_abstract* ttx_type_requirement(void) {
  return &requirement.abstract;
}

perimortem_bool ttx_type_prove(
    const ttx_abstract* candidate,
    ttx_type_view* view) {
  ttx_interface interface =
      ttx_abstract_interface(candidate, ttx_type_requirement());
  if (!ttx_interface_accepts(&interface)) {
    return PERIMORTEM_FALSE;
  }
  view->identity = candidate;
  view->operations = (const ttx_type_operations*)interface.operations;
  return PERIMORTEM_TRUE;
}

const ttx_layout* ttx_type_layout(const ttx_type_view* type) {
  return type->operations->layout(type->identity);
}
