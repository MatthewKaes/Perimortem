// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#ifndef TTX_MODEL_REQUIREMENT_INTERNAL_H
#define TTX_MODEL_REQUIREMENT_INTERNAL_H

#include "ttx/concept/abstract.h"

struct ttx_model_requirement {
  struct ttx_abstract abstract;
  struct perimortem_bytes name;
};

extern const struct ttx_abstract_operations ttx_model_requirement_operations;

#endif
