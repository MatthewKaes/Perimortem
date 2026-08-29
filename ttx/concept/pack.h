// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#ifndef TTX_CONCEPT_PACK_H
#define TTX_CONCEPT_PACK_H

#include "ttx/concept/layout.h"

typedef struct ttx_pack ttx_pack;
typedef struct ttx_pack_operations {
  const ttx_layout* (*layout)(const ttx_pack* self);
} ttx_pack_operations;

struct ttx_pack {
  const ttx_pack_operations* operations;
};

PERIMORTEM_EXTERN_C const ttx_layout* ttx_pack_layout(const ttx_pack* pack);

#endif
