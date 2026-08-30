// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#ifndef TETRODOTOXIN_LANGUAGE_PRODUCT_H
#define TETRODOTOXIN_LANGUAGE_PRODUCT_H

#include "ttx/concept/abstract.h"

struct tetrodotoxin_product_operations {
  struct ttx_interface_operations interface;
  struct perimortem_bytes (*value)(const struct ttx_abstract* identity);
};

struct tetrodotoxin_product_view {
  const struct ttx_abstract* identity;
  const struct tetrodotoxin_product_operations* operations;
};

PERIMORTEM_EXTERN_C const struct ttx_abstract*
    tetrodotoxin_product_requirement(void);
PERIMORTEM_EXTERN_C perimortem_bool tetrodotoxin_product_prove(
    const struct ttx_abstract* candidate,
    struct tetrodotoxin_product_view* view);
PERIMORTEM_EXTERN_C struct perimortem_bytes tetrodotoxin_product_value(
    const struct tetrodotoxin_product_view* product);
PERIMORTEM_EXTERN_C ttx_interface_relation tetrodotoxin_product_relation(
    const struct ttx_abstract* requirement,
    const struct ttx_abstract* candidate);

#endif
