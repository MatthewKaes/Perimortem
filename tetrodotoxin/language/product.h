// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#ifndef TETRODOTOXIN_LANGUAGE_PRODUCT_H
#define TETRODOTOXIN_LANGUAGE_PRODUCT_H

#include "ttx/concept/abstract.h"

typedef struct tetrodotoxin_product_operations {
  ttx_interface_operations interface;
  perimortem_bytes (*value)(const ttx_abstract* identity);
} tetrodotoxin_product_operations;

typedef struct tetrodotoxin_product_view {
  const ttx_abstract* identity;
  const tetrodotoxin_product_operations* operations;
} tetrodotoxin_product_view;

PERIMORTEM_EXTERN_C const ttx_abstract* tetrodotoxin_product_requirement(void);
PERIMORTEM_EXTERN_C perimortem_bool tetrodotoxin_product_prove(
    const ttx_abstract* candidate,
    tetrodotoxin_product_view* view);
PERIMORTEM_EXTERN_C perimortem_bytes tetrodotoxin_product_value(
    const tetrodotoxin_product_view* product);
PERIMORTEM_EXTERN_C ttx_interface_relation tetrodotoxin_product_relation(
    const ttx_abstract* requirement,
    const ttx_abstract* candidate);

#endif
