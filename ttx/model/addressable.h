// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#ifndef TTX_MODEL_ADDRESSABLE_H
#define TTX_MODEL_ADDRESSABLE_H

#include "ttx/concept/abstract.h"

struct ttx_addressable_operations {
  struct ttx_interface_operations interface;
  const struct ttx_abstract* (*type)(const struct ttx_abstract* identity);
};

struct ttx_addressable_view {
  const struct ttx_abstract* identity;
  const struct ttx_addressable_operations* operations;
};

PERIMORTEM_EXTERN_C const struct ttx_abstract*
    ttx_addressable_requirement(void);
PERIMORTEM_EXTERN_C perimortem_bool ttx_addressable_prove(
    const struct ttx_abstract* candidate,
    struct ttx_addressable_view* view);
PERIMORTEM_EXTERN_C const struct ttx_abstract* ttx_addressable_type(
    const struct ttx_addressable_view* addressable);

#endif
