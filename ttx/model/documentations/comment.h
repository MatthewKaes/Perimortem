// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#ifndef TTX_MODEL_DOCUMENTATIONS_COMMENT_H
#define TTX_MODEL_DOCUMENTATIONS_COMMENT_H

#include "ttx/concept/documentation.h"

typedef struct ttx_comment_documentation {
  ttx_documentation documentation;
  perimortem_bytes line;
} ttx_comment_documentation;

PERIMORTEM_EXTERN_C void ttx_comment_documentation_initialize(
    ttx_comment_documentation* documentation,
    perimortem_bytes line);

#endif
