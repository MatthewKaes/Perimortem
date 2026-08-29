// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#ifndef TTX_MODEL_LAYOUTS_COMMON_INTERNAL_H
#define TTX_MODEL_LAYOUTS_COMMON_INTERNAL_H

#include "ttx/concept/layout.h"

perimortem_bool ttx_layout_entries_fit(
    const ttx_layout* source,
    const ttx_layout* target);
perimortem_bool ttx_layout_entry_fits(
    const ttx_abstract* source,
    const ttx_abstract* target);

#endif
