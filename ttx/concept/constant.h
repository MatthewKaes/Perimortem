// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#ifndef TTX_CONCEPT_CONSTANT_H
#define TTX_CONCEPT_CONSTANT_H

#include "ttx/concept/abstract.h"

typedef struct ttx_constant_view {
  const ttx_abstract* identity;
  const ttx_abstract_operations* operations;
} ttx_constant_view;

PERIMORTEM_EXTERN_C const ttx_abstract* ttx_constant_requirement(void);
PERIMORTEM_EXTERN_C perimortem_bool ttx_constant_prove(
    const ttx_abstract* candidate,
    ttx_constant_view* view);

#endif
