// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#ifndef TTX_MODEL_DOCUMENTATIONS_COMMENT_H
#define TTX_MODEL_DOCUMENTATIONS_COMMENT_H

#include "ttx/concept/documentation.h"

struct ttx_comment_documentation {
  struct ttx_documentation documentation;
  struct perimortem_bytes line;
};

PERIMORTEM_EXTERN_C void ttx_comment_documentation_initialize(
    struct ttx_comment_documentation* documentation,
    struct perimortem_bytes line);

#endif
