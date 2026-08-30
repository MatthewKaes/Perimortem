// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#ifndef TTX_MODEL_LAYOUTS_RANGED_H
#define TTX_MODEL_LAYOUTS_RANGED_H

#include "ttx/concept/layout.h"

struct ttx_ranged_layout {
  struct ttx_layout layout;
  const struct ttx_abstract* entry;
  perimortem_count count;
};

PERIMORTEM_EXTERN_C void ttx_ranged_layout_initialize(
    struct ttx_ranged_layout* layout,
    const struct ttx_abstract* entry,
    perimortem_count count);

#endif
