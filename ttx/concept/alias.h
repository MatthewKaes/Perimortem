// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#ifndef TTX_CONCEPT_ALIAS_H
#define TTX_CONCEPT_ALIAS_H

#include "ttx/concept/abstract.h"

// Alias preserves one local name and Documentation while representing another
// borrowed identity. Only resolve traverses that edge, which keeps contextual
// questions with the selected owner instead of turning Alias into a forwarding
// proxy for every category operation.
struct ttx_alias_view {
  const struct ttx_abstract* identity;
  const struct ttx_abstract_operations* operations;
};

PERIMORTEM_EXTERN_C const struct ttx_abstract* ttx_alias_requirement(void);
PERIMORTEM_EXTERN_C perimortem_bool ttx_alias_prove(
    const struct ttx_abstract* candidate,
    struct ttx_alias_view* view);

#endif
