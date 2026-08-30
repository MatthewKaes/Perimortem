// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "ttx/model/layouts/value.h"

#include <stddef.h>

#include "ttx/model/layouts/common_internal.h"

#define TTX_CONTAINER_OF(pointer, type, member) \
  ((type*)((uint8_t*)(pointer)-offsetof(type, member)))

static void visit(
    const struct ttx_layout* base,
    struct ttx_abstract_callable* visitor) {
  const struct ttx_value_layout* self =
      TTX_CONTAINER_OF(base, const struct ttx_value_layout, layout);
  ttx_abstract_callable_call(visitor, self->value);
}

static perimortem_bool fits(
    const struct ttx_layout* source,
    const struct ttx_layout* target) {
  return ttx_layout_entries_fit(source, target);
}

static const struct ttx_layout_operations operations = {
    .visit = visit,
    .fits = fits,
};

void ttx_value_layout_initialize(
    struct ttx_value_layout* layout,
    const struct ttx_abstract* value) {
  layout->layout.operations = &operations;
  layout->layout.named = 0;
  layout->value = value;
}
