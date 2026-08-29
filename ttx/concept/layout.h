// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#ifndef TTX_CONCEPT_LAYOUT_H
#define TTX_CONCEPT_LAYOUT_H

#include "ttx/concept/callable.h"

typedef struct ttx_layout ttx_layout;
typedef struct ttx_named_layout_view ttx_named_layout_view;
typedef struct ttx_named_layout_operations ttx_named_layout_operations;
typedef struct ttx_layout_operations {
  void (*visit)(const ttx_layout* self, ttx_abstract_callable* visitor);
  perimortem_bool (*fits)(
      const ttx_layout* self,
      const ttx_layout* target);
} ttx_layout_operations;

struct ttx_layout {
  const ttx_layout_operations* operations;
  const ttx_named_layout_operations* named;
};

PERIMORTEM_EXTERN_C void ttx_layout_visit(
    const ttx_layout* layout,
    ttx_abstract_callable* visitor);
PERIMORTEM_EXTERN_C perimortem_bool ttx_layout_fits(
    const ttx_layout* source,
    const ttx_layout* target);

#endif
