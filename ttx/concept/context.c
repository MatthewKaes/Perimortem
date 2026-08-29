// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "ttx/concept/context.h"

const ttx_pack* ttx_context_pack(
    ttx_context* context,
    const ttx_layout* layout) {
  return context->operations->pack(context, layout);
}
