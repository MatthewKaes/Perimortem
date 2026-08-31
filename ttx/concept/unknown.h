// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#ifndef TTX_CONCEPT_UNKNOWN_H
#define TTX_CONCEPT_UNKNOWN_H

#include "ttx/concept/abstract.h"

// Unknown proves that a semantic question is not settled by the current graph.
// It is provisional rather than absent, so a later observation may return a
// real identity or None. The marker view preserves the candidate because proof
// never manufactures another semantic object.
struct ttx_unknown_view {
  const struct ttx_abstract* identity;
  const struct ttx_abstract_operations* operations;
};

PERIMORTEM_EXTERN_C const struct ttx_abstract* ttx_unknown_requirement(void);
PERIMORTEM_EXTERN_C perimortem_bool ttx_unknown_prove(
    const struct ttx_abstract* candidate,
    struct ttx_unknown_view* view);

#endif
