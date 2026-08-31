// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#ifndef TTX_MODEL_LAYOUTS_NAMED_H
#define TTX_MODEL_LAYOUTS_NAMED_H

#include "ttx/concept/layout.h"

// Named is an optional Layout witness rather than a field in every Layout.
// Callers prove this exact view before observing names, which keeps Value,
// Fluid, Ranged, and future Layouts independent from naming policy.
struct ttx_named_layout_operations {
  struct ttx_layout_interface_operations interface;
  void (*visit)(
      const struct ttx_layout* layout,
      struct ttx_named_abstract_callable* visitor);
};

struct ttx_named_layout_view {
  const struct ttx_layout* layout;
  const struct ttx_named_layout_operations* operations;
};

struct ttx_named_layout {
  struct ttx_layout layout;
  const struct ttx_layout* source;
  const struct perimortem_bytes* names;
  perimortem_count count;
};

PERIMORTEM_EXTERN_C void ttx_named_layout_initialize(
    struct ttx_named_layout* layout,
    const struct ttx_layout* source,
    const struct perimortem_bytes* names,
    perimortem_count count);

PERIMORTEM_EXTERN_C const struct ttx_layout_interface_requirement*
    ttx_named_layout_requirement(void);

PERIMORTEM_EXTERN_C perimortem_bool ttx_named_layout_prove(
    const struct ttx_layout* layout,
    struct ttx_named_layout_view* named);

PERIMORTEM_EXTERN_C void ttx_named_layout_visit(
    const struct ttx_named_layout_view* layout,
    struct ttx_named_abstract_callable* visitor);

#endif
