// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#ifndef TTX_CONCEPT_REQUIREMENT_INTERNAL_H
#define TTX_CONCEPT_REQUIREMENT_INTERNAL_H

#include "ttx/concept/abstract.h"

struct ttx_requirement {
  struct ttx_abstract abstract;
  struct perimortem_bytes name;
};

extern const struct ttx_abstract_operations ttx_requirement_operations;

perimortem_bool ttx_requirement_prove(
    const struct ttx_abstract* requirement,
    const struct ttx_abstract* candidate,
    const struct ttx_abstract** identity,
    const struct ttx_abstract_operations** operations);

#endif
