// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#ifndef TTX_CONCEPT_LAYOUT_H
#define TTX_CONCEPT_LAYOUT_H

#include "ttx/concept/callable.h"

struct ttx_layout;
struct ttx_named_layout_view;
struct ttx_named_layout_operations;
struct ttx_layout_operations {
  void (*visit)(
      const struct ttx_layout* self,
      struct ttx_abstract_callable* visitor);
  perimortem_bool (*fits)(
      const struct ttx_layout* self,
      const struct ttx_layout* target);
};

struct ttx_layout {
  const struct ttx_layout_operations* operations;
  const struct ttx_named_layout_operations* named;
};

PERIMORTEM_EXTERN_C void ttx_layout_visit(
    const struct ttx_layout* layout,
    struct ttx_abstract_callable* visitor);
PERIMORTEM_EXTERN_C perimortem_bool ttx_layout_fits(
    const struct ttx_layout* source,
    const struct ttx_layout* target);

#endif
