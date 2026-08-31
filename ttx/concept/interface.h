// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#ifndef TTX_CONCEPT_INTERFACE_H
#define TTX_CONCEPT_INTERFACE_H

#include "perimortem/core/perimortem.h"

struct ttx_abstract;

// Interface records the strength of one explicit semantic comparison.
// Satisfaction is directional, while equivalence means the concrete owner has
// proved the relation in both directions.
typedef uint8_t ttx_interface_relation;

enum {
  TTX_INTERFACE_REJECTED = 0,
  TTX_INTERFACE_SATISFIED = 1,
  TTX_INTERFACE_EQUIVALENT = 2,
};

// Category operation tables place Interface operations first. C preserves the
// address of that first member, so successful negotiation can expose the full
// category witness without a native type code, adjusted object pointer, or
// registry lookup.
struct ttx_interface_operations {
  ttx_interface_relation (*negotiate)(
      const struct ttx_abstract* requirement,
      const struct ttx_abstract* candidate);
};

// COM names an immutable binary interface with an external identifier. TTX can
// use the real requirement Abstract instead because both participants already
// live in one semantic graph. The result borrows those identities and carries
// no wrapper object or ownership claim.
struct ttx_interface {
  const struct ttx_abstract* requirement;
  const struct ttx_abstract* candidate;
  const struct ttx_interface_operations* operations;
};

PERIMORTEM_EXTERN_C ttx_interface_relation
    ttx_interface_negotiate(const struct ttx_interface* interface);

PERIMORTEM_EXTERN_C perimortem_bool
    ttx_interface_accepts(const struct ttx_interface* interface);

PERIMORTEM_EXTERN_C struct ttx_interface ttx_interface_rejected(
    const struct ttx_abstract* requirement,
    const struct ttx_abstract* candidate);

PERIMORTEM_EXTERN_C struct ttx_interface ttx_interface_satisfied(
    const struct ttx_abstract* requirement,
    const struct ttx_abstract* candidate,
    const struct ttx_interface_operations* operations);

PERIMORTEM_EXTERN_C const struct ttx_interface_operations*
    ttx_interface_marker_operations(void);

#endif
