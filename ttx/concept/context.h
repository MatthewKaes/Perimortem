// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#ifndef TTX_CONCEPT_CONTEXT_H
#define TTX_CONCEPT_CONTEXT_H

#include "ttx/concept/pack.h"

typedef struct ttx_context ttx_context;
typedef struct ttx_context_operations {
  const ttx_pack* (*pack)(ttx_context* self, const ttx_layout* layout);
} ttx_context_operations;

struct ttx_context {
  const ttx_context_operations* operations;
};

PERIMORTEM_EXTERN_C const ttx_pack* ttx_context_pack(
    ttx_context* context,
    const ttx_layout* layout);

#endif
