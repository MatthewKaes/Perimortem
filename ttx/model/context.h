// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#ifndef TTX_MODEL_CONTEXT_H
#define TTX_MODEL_CONTEXT_H

#include "ttx/concept/context.h"
#include "ttx/model/layouts/fluid.h"
#include "ttx/model/layouts/named.h"
#include "ttx/model/pack.h"

struct ttx_model_context_storage {
  struct ttx_model_pack* packs;
  struct ttx_fluid_layout* layouts;
  struct ttx_named_layout* named_layouts;
  const struct ttx_abstract** entries;
  struct perimortem_bytes* names;
  uint8_t* name_bytes;
  perimortem_count pack_capacity;
  perimortem_count entry_capacity;
  perimortem_count name_capacity;
  perimortem_count name_byte_capacity;
};

struct ttx_model_context {
  struct ttx_context context;
  struct ttx_model_context_storage storage;
  perimortem_count pack_count;
  perimortem_count entry_count;
  perimortem_count name_count;
  perimortem_count name_byte_count;
};

PERIMORTEM_EXTERN_C void ttx_model_context_initialize(
    struct ttx_model_context* context,
    struct ttx_model_context_storage storage);

#endif
