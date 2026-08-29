// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#ifndef TTX_MODEL_LAYOUTS_FLUID_H
#define TTX_MODEL_LAYOUTS_FLUID_H

#include "ttx/concept/layout.h"

typedef struct ttx_fluid_layout {
  ttx_layout layout;
  const ttx_abstract* const* entries;
  perimortem_count count;
} ttx_fluid_layout;

PERIMORTEM_EXTERN_C void ttx_fluid_layout_initialize(
    ttx_fluid_layout* layout,
    const ttx_abstract* const* entries,
    perimortem_count count);

#endif
