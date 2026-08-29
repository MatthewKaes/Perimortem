// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "ttx/model/addressable.h"

#include "ttx/model/requirement_internal.h"

static const uint8_t name[] = "Addressable";

static const ttx_model_requirement requirement = {
    .abstract = {.operations = &ttx_model_requirement_operations},
    .name = {.data = name, .size = sizeof(name) - 1},
};

const ttx_abstract* ttx_addressable_requirement(void) {
  return &requirement.abstract;
}

perimortem_bool ttx_addressable_prove(
    const ttx_abstract* candidate,
    ttx_addressable_view* view) {
  ttx_interface interface =
      ttx_abstract_interface(candidate, ttx_addressable_requirement());
  if (!ttx_interface_accepts(&interface)) {
    return PERIMORTEM_FALSE;
  }
  view->identity = candidate;
  view->operations =
      (const ttx_addressable_operations*)interface.operations;
  return PERIMORTEM_TRUE;
}

const ttx_abstract* ttx_addressable_type(
    const ttx_addressable_view* addressable) {
  return addressable->operations->type(addressable->identity);
}
