// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#ifndef TTX_MODEL_DOCUMENTATIONS_MERGED_H
#define TTX_MODEL_DOCUMENTATIONS_MERGED_H

#include "ttx/concept/documentation.h"

typedef struct ttx_merged_documentation {
  ttx_documentation documentation;
  const ttx_documentation* first;
  const ttx_documentation* second;
} ttx_merged_documentation;

PERIMORTEM_EXTERN_C void ttx_merged_documentation_initialize(
    ttx_merged_documentation* documentation,
    const ttx_documentation* first,
    const ttx_documentation* second);

#endif
