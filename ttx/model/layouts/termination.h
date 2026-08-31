// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#ifndef TTX_MODEL_LAYOUTS_TERMINATION_H
#define TTX_MODEL_LAYOUTS_TERMINATION_H

#include "ttx/model/type.h"

PERIMORTEM_EXTERN_C perimortem_bool
    ttx_type_layout_terminates(const struct ttx_type_view* type);

#endif
