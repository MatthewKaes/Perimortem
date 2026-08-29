// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#ifndef TTX_MODEL_TYPE_H
#define TTX_MODEL_TYPE_H

#include "ttx/concept/abstract.h"
#include "ttx/concept/layout.h"

typedef struct ttx_type_operations {
  ttx_interface_operations interface;
  const ttx_layout* (*layout)(const ttx_abstract* identity);
} ttx_type_operations;

typedef struct ttx_type_view {
  const ttx_abstract* identity;
  const ttx_type_operations* operations;
} ttx_type_view;

PERIMORTEM_EXTERN_C const ttx_abstract* ttx_type_requirement(void);
PERIMORTEM_EXTERN_C perimortem_bool ttx_type_prove(
    const ttx_abstract* candidate,
    ttx_type_view* view);
PERIMORTEM_EXTERN_C const ttx_layout* ttx_type_layout(const ttx_type_view* type);

#endif
