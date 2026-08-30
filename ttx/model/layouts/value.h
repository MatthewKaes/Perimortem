// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#ifndef TTX_MODEL_LAYOUTS_VALUE_H
#define TTX_MODEL_LAYOUTS_VALUE_H

#include "ttx/concept/layout.h"

struct ttx_value_layout {
  struct ttx_layout layout;
  const struct ttx_abstract* value;
};

PERIMORTEM_EXTERN_C void ttx_value_layout_initialize(
    struct ttx_value_layout* layout,
    const struct ttx_abstract* value);

#endif
