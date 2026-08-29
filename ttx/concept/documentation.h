// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#ifndef TTX_CONCEPT_DOCUMENTATION_H
#define TTX_CONCEPT_DOCUMENTATION_H

#include "ttx/concept/callable.h"

typedef struct ttx_documentation ttx_documentation;
typedef struct ttx_documentation_operations {
  void (*visit)(const ttx_documentation* self, ttx_bytes_callable* visitor);
} ttx_documentation_operations;

struct ttx_documentation {
  const ttx_documentation_operations* operations;
};

PERIMORTEM_EXTERN_C void ttx_documentation_visit(
    const ttx_documentation* documentation,
    ttx_bytes_callable* visitor);

PERIMORTEM_EXTERN_C const ttx_documentation* ttx_documentation_empty(void);

#endif
