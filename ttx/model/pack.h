// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#ifndef TTX_MODEL_PACK_H
#define TTX_MODEL_PACK_H

#include "ttx/concept/pack.h"

typedef struct ttx_model_pack {
  ttx_pack pack;
  const ttx_layout* layout;
} ttx_model_pack;

#endif
