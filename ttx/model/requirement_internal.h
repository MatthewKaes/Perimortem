// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#ifndef TTX_MODEL_REQUIREMENT_INTERNAL_H
#define TTX_MODEL_REQUIREMENT_INTERNAL_H

#include "ttx/concept/abstract.h"

typedef struct ttx_model_requirement {
  ttx_abstract abstract;
  perimortem_bytes name;
} ttx_model_requirement;

extern const ttx_abstract_operations ttx_model_requirement_operations;

#endif
