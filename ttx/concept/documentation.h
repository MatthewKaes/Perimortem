// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#ifndef TTX_CONCEPT_DOCUMENTATION_H
#define TTX_CONCEPT_DOCUMENTATION_H

#include "ttx/concept/callable.h"

struct ttx_documentation;
struct ttx_documentation_operations {
  void (*visit)(
      const struct ttx_documentation* self,
      struct ttx_bytes_callable* visitor);
};

struct ttx_documentation {
  const struct ttx_documentation_operations* operations;
};

PERIMORTEM_EXTERN_C void ttx_documentation_visit(
    const struct ttx_documentation* documentation,
    struct ttx_bytes_callable* visitor);

PERIMORTEM_EXTERN_C const struct ttx_documentation* ttx_documentation_empty(
    void);

#endif
