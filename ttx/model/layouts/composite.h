// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#ifndef TTX_MODEL_LAYOUTS_COMPOSITE_H
#define TTX_MODEL_LAYOUTS_COMPOSITE_H

#include "ttx/concept/layout.h"

struct ttx_composite_layout {
  struct ttx_layout layout;
  const struct ttx_layout* first;
  const struct ttx_layout* second;
};

PERIMORTEM_EXTERN_C void ttx_composite_layout_initialize(
    struct ttx_composite_layout* layout,
    const struct ttx_layout* first,
    const struct ttx_layout* second);

#endif
