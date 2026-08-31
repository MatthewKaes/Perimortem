// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#ifndef TTX_MODEL_TYPE_H
#define TTX_MODEL_TYPE_H

#include "ttx/concept/abstract.h"
#include "ttx/concept/layout.h"

// Type adds one total semantic Layout to the original Abstract. The Interface
// prefix lets category proof return this complete witness while preserving the
// candidate identity. Physical size, alignment, storage, and defaults remain
// with concrete languages and Terminals because equal Layout does not make two
// Types identical.
struct ttx_type_operations {
  struct ttx_interface_operations interface;
  const struct ttx_layout* (*layout)(const struct ttx_abstract* identity);
};

struct ttx_type_view {
  const struct ttx_abstract* identity;
  const struct ttx_type_operations* operations;
};

PERIMORTEM_EXTERN_C const struct ttx_abstract* ttx_type_requirement(void);
PERIMORTEM_EXTERN_C perimortem_bool ttx_type_prove(
    const struct ttx_abstract* candidate,
    struct ttx_type_view* view);
PERIMORTEM_EXTERN_C const struct ttx_layout* ttx_type_layout(
    const struct ttx_type_view* type);

#endif
