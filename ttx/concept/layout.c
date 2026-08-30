// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "ttx/concept/layout.h"

void ttx_layout_visit(
    const struct ttx_layout* layout,
    struct ttx_abstract_callable* visitor) {
  layout->operations->visit(layout, visitor);
}

perimortem_bool ttx_layout_fits(
    const struct ttx_layout* source,
    const struct ttx_layout* target) {
  return source->operations->fits(source, target);
}
