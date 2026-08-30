// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "ttx/concept/context.h"

const struct ttx_pack* ttx_context_pack(
    struct ttx_context* context,
    const struct ttx_layout* layout) {
  return context->operations->pack(context, layout);
}
