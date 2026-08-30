// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#ifndef TTX_MODEL_CALLABLE_H
#define TTX_MODEL_CALLABLE_H

#include "ttx/concept/abstract.h"
#include "ttx/concept/layout.h"

struct ttx_callable_operations {
  struct ttx_interface_operations interface;
  const struct ttx_layout* (*parameters)(const struct ttx_abstract* identity);
  const struct ttx_layout* (*results)(const struct ttx_abstract* identity);
};

struct ttx_callable_view {
  const struct ttx_abstract* identity;
  const struct ttx_callable_operations* operations;
};

PERIMORTEM_EXTERN_C const struct ttx_abstract* ttx_callable_requirement(void);
PERIMORTEM_EXTERN_C perimortem_bool ttx_callable_prove(
    const struct ttx_abstract* candidate,
    struct ttx_callable_view* view);
PERIMORTEM_EXTERN_C const struct ttx_layout* ttx_callable_parameters(
    const struct ttx_callable_view* callable);
PERIMORTEM_EXTERN_C const struct ttx_layout* ttx_callable_results(
    const struct ttx_callable_view* callable);

#endif
