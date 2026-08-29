// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#ifndef TTX_MODEL_LAYOUTS_VALUE_H
#define TTX_MODEL_LAYOUTS_VALUE_H

#include "ttx/concept/layout.h"

typedef struct ttx_value_layout {
  ttx_layout layout;
  const ttx_abstract* value;
} ttx_value_layout;

PERIMORTEM_EXTERN_C void ttx_value_layout_initialize(
    ttx_value_layout* layout,
    const ttx_abstract* value);

#endif
