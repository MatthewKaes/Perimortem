// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#ifndef TTX_MODEL_PACK_H
#define TTX_MODEL_PACK_H

#include "ttx/concept/pack.h"

struct ttx_model_pack {
  struct ttx_pack pack;
  const struct ttx_layout* layout;
};

#endif
