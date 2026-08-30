// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#ifndef TTX_MODEL_DOCUMENTATIONS_MERGED_H
#define TTX_MODEL_DOCUMENTATIONS_MERGED_H

#include "ttx/concept/documentation.h"

struct ttx_merged_documentation {
  struct ttx_documentation documentation;
  const struct ttx_documentation* first;
  const struct ttx_documentation* second;
};

PERIMORTEM_EXTERN_C void ttx_merged_documentation_initialize(
    struct ttx_merged_documentation* documentation,
    const struct ttx_documentation* first,
    const struct ttx_documentation* second);

#endif
