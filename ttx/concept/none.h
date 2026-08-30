// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#ifndef TTX_CONCEPT_NONE_H
#define TTX_CONCEPT_NONE_H

#include "ttx/concept/constant.h"

struct ttx_none_view {
  const struct ttx_abstract* identity;
  const struct ttx_abstract_operations* operations;
};

PERIMORTEM_EXTERN_C const struct ttx_abstract* ttx_none_requirement(void);
PERIMORTEM_EXTERN_C perimortem_bool ttx_none_prove(
    const struct ttx_abstract* candidate,
    struct ttx_none_view* view);

#endif
