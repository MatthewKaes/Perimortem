// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include <stddef.h>

#include "ttx/model/pack_internal.h"

#define TTX_CONTAINER_OF(pointer, type, member) \
  ((type*)((uint8_t*)(pointer) - offsetof(type, member)))

static const struct ttx_layout* layout(const struct ttx_pack* base) {
  const struct ttx_model_pack* self =
      TTX_CONTAINER_OF(base, const struct ttx_model_pack, pack);
  return self->layout;
}

static const struct ttx_pack_operations operations = {
  .layout = layout,
};

void ttx_model_pack_initialize(
    struct ttx_model_pack* pack,
    const struct ttx_layout* pack_layout) {
  pack->pack.operations = &operations;
  pack->layout = pack_layout;
}
