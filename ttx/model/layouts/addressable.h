// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#ifndef TTX_MODEL_LAYOUTS_ADDRESSABLE_H
#define TTX_MODEL_LAYOUTS_ADDRESSABLE_H

#include "ttx/model/addressable.h"
#include "ttx/model/layouts/fluid.h"
#include "ttx/model/layouts/named.h"

typedef struct ttx_layout_addressable {
  ttx_abstract abstract;
  perimortem_bytes name;
  const ttx_abstract* type;
  ttx_addressable_operations addressable;
} ttx_layout_addressable;

PERIMORTEM_EXTERN_C void ttx_layout_addressable_initialize(
    ttx_layout_addressable* addressable,
    perimortem_bytes name,
    const ttx_abstract* type);

typedef struct ttx_addressable_layout {
  ttx_named_layout named;
  ttx_fluid_layout entries;
} ttx_addressable_layout;

PERIMORTEM_EXTERN_C void ttx_addressable_layout_initialize(
    ttx_addressable_layout* layout,
    const ttx_abstract* const* entries,
    const perimortem_bytes* names,
    perimortem_count count);

#endif
