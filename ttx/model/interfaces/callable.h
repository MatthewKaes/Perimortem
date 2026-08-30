// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#ifndef TTX_MODEL_INTERFACES_CALLABLE_H
#define TTX_MODEL_INTERFACES_CALLABLE_H

#include "ttx/model/callable.h"

PERIMORTEM_EXTERN_C ttx_interface_relation ttx_callable_negotiate(
    const struct ttx_abstract* requirement,
    const struct ttx_abstract* candidate);

#endif
