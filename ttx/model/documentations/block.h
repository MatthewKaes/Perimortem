// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#ifndef TTX_MODEL_DOCUMENTATIONS_BLOCK_H
#define TTX_MODEL_DOCUMENTATIONS_BLOCK_H

#include "ttx/concept/documentation.h"

struct ttx_block_documentation {
  struct ttx_documentation documentation;
  const struct perimortem_bytes* lines;
  perimortem_count count;
};

PERIMORTEM_EXTERN_C void ttx_block_documentation_initialize(
    struct ttx_block_documentation* documentation,
    const struct perimortem_bytes* lines,
    perimortem_count count);

#endif
