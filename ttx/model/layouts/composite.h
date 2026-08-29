// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#ifndef TTX_MODEL_LAYOUTS_COMPOSITE_H
#define TTX_MODEL_LAYOUTS_COMPOSITE_H

#include "ttx/concept/layout.h"

typedef struct ttx_composite_layout {
  ttx_layout layout;
  const ttx_layout* first;
  const ttx_layout* second;
} ttx_composite_layout;

PERIMORTEM_EXTERN_C void ttx_composite_layout_initialize(
    ttx_composite_layout* layout,
    const ttx_layout* first,
    const ttx_layout* second);

#endif
