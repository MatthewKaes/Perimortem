// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#ifndef TETRODOTOXIN_LIBRARY_LANGUAGE_MODEL_INVOCATION_H
#define TETRODOTOXIN_LIBRARY_LANGUAGE_MODEL_INVOCATION_H

#include "ttx/concept/abstract.h"
#include "ttx/concept/pack.h"

// Invocation is Library's executable Callable protocol. It receives the
// current receiver and argument flows and returns their real produced Pack.
// Whether those producers currently prove Constant is a question for the
// caller, not part of invocation.
struct ttx_library_invocation_operations {
  struct ttx_interface_operations interface;
  const struct ttx_pack* (*invoke)(
      const struct ttx_abstract* callable,
      const struct ttx_pack* receiver,
      const struct ttx_pack* arguments);
};

struct ttx_library_invocation_view {
  const struct ttx_abstract* identity;
  const struct ttx_library_invocation_operations* operations;
};

PERIMORTEM_EXTERN_C const struct ttx_abstract*
    ttx_library_invocation_requirement(void);
PERIMORTEM_EXTERN_C perimortem_bool ttx_library_invocation_prove(
    const struct ttx_abstract* candidate,
    struct ttx_library_invocation_view* view);
PERIMORTEM_EXTERN_C const struct ttx_pack* ttx_library_invoke(
    const struct ttx_library_invocation_view* callable,
    const struct ttx_pack* receiver,
    const struct ttx_pack* arguments);
PERIMORTEM_EXTERN_C ttx_interface_relation ttx_library_invocation_relation(
    const struct ttx_abstract* requirement,
    const struct ttx_abstract* candidate);

#endif
