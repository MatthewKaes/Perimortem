// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#ifndef TTX_MODEL_LAYOUTS_ADDRESSABLE_H
#define TTX_MODEL_LAYOUTS_ADDRESSABLE_H

#include "ttx/model/addressable.h"
#include "ttx/model/layouts/fluid.h"
#include "ttx/model/layouts/named.h"

struct ttx_layout_addressable {
  struct ttx_abstract abstract;
  struct perimortem_bytes name;
  const struct ttx_abstract* type;
  struct ttx_addressable_operations addressable;
};

PERIMORTEM_EXTERN_C void ttx_layout_addressable_initialize(
    struct ttx_layout_addressable* addressable,
    struct perimortem_bytes name,
    const struct ttx_abstract* type);

struct ttx_addressable_layout {
  struct ttx_named_layout named;
  struct ttx_fluid_layout entries;
};

PERIMORTEM_EXTERN_C void ttx_addressable_layout_initialize(
    struct ttx_addressable_layout* layout,
    const struct ttx_abstract* const* entries,
    const struct perimortem_bytes* names,
    perimortem_count count);

#endif
