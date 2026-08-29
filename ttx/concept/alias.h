// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#ifndef TTX_CONCEPT_ALIAS_H
#define TTX_CONCEPT_ALIAS_H

#include "ttx/concept/abstract.h"

typedef struct ttx_alias_view {
  const ttx_abstract* identity;
  const ttx_abstract_operations* operations;
} ttx_alias_view;

PERIMORTEM_EXTERN_C const ttx_abstract* ttx_alias_requirement(void);
PERIMORTEM_EXTERN_C perimortem_bool ttx_alias_prove(
    const ttx_abstract* candidate,
    ttx_alias_view* view);

#endif
