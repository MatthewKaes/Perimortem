// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#ifndef TTX_MODEL_ADDRESSABLE_H
#define TTX_MODEL_ADDRESSABLE_H

#include "ttx/concept/abstract.h"

typedef struct ttx_addressable_operations {
  ttx_interface_operations interface;
  const ttx_abstract* (*type)(const ttx_abstract* identity);
} ttx_addressable_operations;

typedef struct ttx_addressable_view {
  const ttx_abstract* identity;
  const ttx_addressable_operations* operations;
} ttx_addressable_view;

PERIMORTEM_EXTERN_C const ttx_abstract* ttx_addressable_requirement(void);
PERIMORTEM_EXTERN_C perimortem_bool ttx_addressable_prove(
    const ttx_abstract* candidate,
    ttx_addressable_view* view);
PERIMORTEM_EXTERN_C const ttx_abstract* ttx_addressable_type(
    const ttx_addressable_view* addressable);

#endif
