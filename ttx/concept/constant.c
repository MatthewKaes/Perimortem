// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "ttx/concept/constant.h"

#include "ttx/concept/requirement_internal.h"

static const uint8_t name[] = "Constant";

static const ttx_requirement requirement = {
    .abstract = {.operations = &ttx_requirement_operations},
    .name = {.data = name, .size = sizeof(name) - 1},
};

const ttx_abstract* ttx_constant_requirement(void) {
  return &requirement.abstract;
}

perimortem_bool ttx_constant_prove(
    const ttx_abstract* candidate,
    ttx_constant_view* view) {
  return ttx_requirement_prove(
      ttx_constant_requirement(),
      candidate,
      &view->identity,
      &view->operations);
}
