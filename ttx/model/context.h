// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#ifndef TTX_MODEL_CONTEXT_H
#define TTX_MODEL_CONTEXT_H

#include "ttx/concept/context.h"
#include "ttx/model/layouts/fluid.h"
#include "ttx/model/layouts/named.h"
#include "ttx/model/pack.h"

typedef struct ttx_model_context_storage {
  ttx_model_pack* packs;
  ttx_fluid_layout* layouts;
  ttx_named_layout* named_layouts;
  const ttx_abstract** entries;
  perimortem_bytes* names;
  uint8_t* name_bytes;
  perimortem_count pack_capacity;
  perimortem_count entry_capacity;
  perimortem_count name_capacity;
  perimortem_count name_byte_capacity;
} ttx_model_context_storage;

typedef struct ttx_model_context {
  ttx_context context;
  ttx_model_context_storage storage;
  perimortem_count pack_count;
  perimortem_count entry_count;
  perimortem_count name_count;
  perimortem_count name_byte_count;
} ttx_model_context;

PERIMORTEM_EXTERN_C void ttx_model_context_initialize(
    ttx_model_context* context,
    ttx_model_context_storage storage);

#endif
