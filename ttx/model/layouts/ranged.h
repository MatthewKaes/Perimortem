// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#ifndef TTX_MODEL_LAYOUTS_RANGED_H
#define TTX_MODEL_LAYOUTS_RANGED_H

#include "ttx/concept/layout.h"

typedef struct ttx_ranged_layout {
  ttx_layout layout;
  const ttx_abstract* entry;
  perimortem_count count;
} ttx_ranged_layout;

PERIMORTEM_EXTERN_C void ttx_ranged_layout_initialize(
    ttx_ranged_layout* layout,
    const ttx_abstract* entry,
    perimortem_count count);

#endif
