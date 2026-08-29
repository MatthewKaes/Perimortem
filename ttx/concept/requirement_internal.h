// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#ifndef TTX_CONCEPT_REQUIREMENT_INTERNAL_H
#define TTX_CONCEPT_REQUIREMENT_INTERNAL_H

#include "ttx/concept/abstract.h"

typedef struct ttx_requirement {
  ttx_abstract abstract;
  perimortem_bytes name;
} ttx_requirement;

extern const ttx_abstract_operations ttx_requirement_operations;

perimortem_bool ttx_requirement_prove(
    const ttx_abstract* requirement,
    const ttx_abstract* candidate,
    const ttx_abstract** identity,
    const ttx_abstract_operations** operations);

#endif
