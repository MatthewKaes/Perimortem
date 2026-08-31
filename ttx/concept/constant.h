// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#ifndef TTX_CONCEPT_CONSTANT_H
#define TTX_CONCEPT_CONSTANT_H

#include "ttx/concept/abstract.h"

// Constant proves that one Abstract is a complete immutable fact for its graph
// lifetime. Derived work may reuse an answer while its current inputs are the
// same exact Constant identities, but a live route is asked again because the
// route itself did not become immutable.
struct ttx_constant_view {
  const struct ttx_abstract* identity;
  const struct ttx_abstract_operations* operations;
};

PERIMORTEM_EXTERN_C const struct ttx_abstract* ttx_constant_requirement(void);
PERIMORTEM_EXTERN_C perimortem_bool ttx_constant_prove(
    const struct ttx_abstract* candidate,
    struct ttx_constant_view* view);

#endif
