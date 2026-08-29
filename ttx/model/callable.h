// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#ifndef TTX_MODEL_CALLABLE_H
#define TTX_MODEL_CALLABLE_H

#include "ttx/concept/abstract.h"
#include "ttx/concept/layout.h"

typedef struct ttx_callable_operations {
  ttx_interface_operations interface;
  const ttx_layout* (*parameters)(const ttx_abstract* identity);
  const ttx_layout* (*results)(const ttx_abstract* identity);
} ttx_callable_operations;

typedef struct ttx_callable_view {
  const ttx_abstract* identity;
  const ttx_callable_operations* operations;
} ttx_callable_view;

PERIMORTEM_EXTERN_C const ttx_abstract* ttx_callable_requirement(void);
PERIMORTEM_EXTERN_C perimortem_bool ttx_callable_prove(
    const ttx_abstract* candidate,
    ttx_callable_view* view);
PERIMORTEM_EXTERN_C const ttx_layout* ttx_callable_parameters(const ttx_callable_view* callable);
PERIMORTEM_EXTERN_C const ttx_layout* ttx_callable_results(const ttx_callable_view* callable);

#endif
