// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#ifndef TTX_MODEL_DOCUMENTATIONS_BLOCK_H
#define TTX_MODEL_DOCUMENTATIONS_BLOCK_H

#include "ttx/concept/documentation.h"

typedef struct ttx_block_documentation {
  ttx_documentation documentation;
  const perimortem_bytes* lines;
  perimortem_count count;
} ttx_block_documentation;

PERIMORTEM_EXTERN_C void ttx_block_documentation_initialize(
    ttx_block_documentation* documentation,
    const perimortem_bytes* lines,
    perimortem_count count);

#endif
