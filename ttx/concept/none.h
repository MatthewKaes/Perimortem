// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#ifndef TTX_CONCEPT_NONE_H
#define TTX_CONCEPT_NONE_H

#include "ttx/concept/constant.h"

typedef struct ttx_none_view {
  const ttx_abstract* identity;
  const ttx_abstract_operations* operations;
} ttx_none_view;

PERIMORTEM_EXTERN_C const ttx_abstract* ttx_none_requirement(void);
PERIMORTEM_EXTERN_C perimortem_bool ttx_none_prove(
    const ttx_abstract* candidate,
    ttx_none_view* view);

#endif
