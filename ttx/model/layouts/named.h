// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#ifndef TTX_MODEL_LAYOUTS_NAMED_H
#define TTX_MODEL_LAYOUTS_NAMED_H

#include "ttx/concept/layout.h"

typedef struct ttx_named_layout_operations {
  void (*visit)(
      const ttx_layout* layout,
      ttx_named_abstract_callable* visitor);
} ttx_named_layout_operations;

struct ttx_named_layout_view {
  const ttx_layout* layout;
  const ttx_named_layout_operations* operations;
};

typedef struct ttx_named_layout {
  ttx_layout layout;
  const ttx_layout* source;
  const perimortem_bytes* names;
  perimortem_count count;
} ttx_named_layout;

PERIMORTEM_EXTERN_C void ttx_named_layout_initialize(
    ttx_named_layout* layout,
    const ttx_layout* source,
    const perimortem_bytes* names,
    perimortem_count count);

PERIMORTEM_EXTERN_C perimortem_bool ttx_named_layout_prove(
    const ttx_layout* layout,
    ttx_named_layout_view* named);

PERIMORTEM_EXTERN_C void ttx_named_layout_visit(
    const ttx_named_layout_view* layout,
    ttx_named_abstract_callable* visitor);

#endif
