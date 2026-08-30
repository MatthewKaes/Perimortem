// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#ifndef TTX_MODEL_LAYOUTS_FLUID_H
#define TTX_MODEL_LAYOUTS_FLUID_H

#include "ttx/concept/layout.h"

struct ttx_fluid_layout {
  struct ttx_layout layout;
  const struct ttx_abstract* const* entries;
  perimortem_count count;
};

PERIMORTEM_EXTERN_C void ttx_fluid_layout_initialize(
    struct ttx_fluid_layout* layout,
    const struct ttx_abstract* const* entries,
    perimortem_count count);

#endif
