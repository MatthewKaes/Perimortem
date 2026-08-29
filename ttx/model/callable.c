// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "ttx/model/callable.h"

#include "ttx/model/requirement_internal.h"

static const uint8_t name[] = "Callable";

static const ttx_model_requirement requirement = {
    .abstract = {.operations = &ttx_model_requirement_operations},
    .name = {.data = name, .size = sizeof(name) - 1},
};

const ttx_abstract* ttx_callable_requirement(void) {
  return &requirement.abstract;
}

perimortem_bool ttx_callable_prove(
    const ttx_abstract* candidate,
    ttx_callable_view* view) {
  ttx_interface interface =
      ttx_abstract_interface(candidate, ttx_callable_requirement());
  if (!ttx_interface_accepts(&interface)) {
    return PERIMORTEM_FALSE;
  }
  view->identity = candidate;
  view->operations = (const ttx_callable_operations*)interface.operations;
  return PERIMORTEM_TRUE;
}

const ttx_layout* ttx_callable_parameters(const ttx_callable_view* callable) {
  return callable->operations->parameters(callable->identity);
}

const ttx_layout* ttx_callable_results(const ttx_callable_view* callable) {
  return callable->operations->results(callable->identity);
}
