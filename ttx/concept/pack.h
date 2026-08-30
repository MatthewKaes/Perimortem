// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#ifndef TTX_CONCEPT_PACK_H
#define TTX_CONCEPT_PACK_H

#include "ttx/concept/layout.h"

struct ttx_pack;
struct ttx_pack_operations {
  const struct ttx_layout* (*layout)(const struct ttx_pack* self);
};

struct ttx_pack {
  const struct ttx_pack_operations* operations;
};

PERIMORTEM_EXTERN_C const struct ttx_layout* ttx_pack_layout(
    const struct ttx_pack* pack);

#endif
