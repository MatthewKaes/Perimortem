// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#ifndef TTX_CONCEPT_UNKNOWN_H
#define TTX_CONCEPT_UNKNOWN_H

#include "ttx/concept/abstract.h"

typedef struct ttx_unknown_view {
  const ttx_abstract* identity;
  const ttx_abstract_operations* operations;
} ttx_unknown_view;

PERIMORTEM_EXTERN_C const ttx_abstract* ttx_unknown_requirement(void);
PERIMORTEM_EXTERN_C perimortem_bool ttx_unknown_prove(
    const ttx_abstract* candidate,
    ttx_unknown_view* view);

#endif
