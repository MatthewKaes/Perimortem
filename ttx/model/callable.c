// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "ttx/model/callable.h"

#include "ttx/model/requirement_internal.h"

static const uint8_t name[] = "Callable";

static const struct ttx_model_requirement requirement = {
    .abstract = {.operations = &ttx_model_requirement_operations},
    .name = {.data = name, .size = sizeof(name) - 1},
};

const struct ttx_abstract* ttx_callable_requirement(void) {
  return &requirement.abstract;
}

perimortem_bool ttx_callable_prove(
    const struct ttx_abstract* candidate,
    struct ttx_callable_view* view) {
  struct ttx_interface interface =
      ttx_abstract_interface(candidate, ttx_callable_requirement());
  if (!ttx_interface_accepts(&interface)) {
    return PERIMORTEM_FALSE;
  }
  view->identity = candidate;
  view->operations = (const struct ttx_callable_operations*)interface.operations;
  return PERIMORTEM_TRUE;
}

const struct ttx_layout* ttx_callable_parameters(const struct ttx_callable_view* callable) {
  return callable->operations->parameters(callable->identity);
}

const struct ttx_layout* ttx_callable_results(const struct ttx_callable_view* callable) {
  return callable->operations->results(callable->identity);
}
