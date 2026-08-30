// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#ifndef TTX_CONCEPT_CONTEXT_H
#define TTX_CONCEPT_CONTEXT_H

#include "ttx/concept/pack.h"

struct ttx_context;
struct ttx_context_operations {
  const struct ttx_pack* (*pack)(
      struct ttx_context* self,
      const struct ttx_layout* layout);
};

struct ttx_context {
  const struct ttx_context_operations* operations;
};

PERIMORTEM_EXTERN_C const struct ttx_pack* ttx_context_pack(
    struct ttx_context* context,
    const struct ttx_layout* layout);

#endif
