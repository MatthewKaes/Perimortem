// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "ttx/model/layouts/ranged.h"

#include <stddef.h>

#include "ttx/model/layouts/common_internal.h"

#define TTX_CONTAINER_OF(pointer, type, member) \
  ((type*)((uint8_t*)(pointer)-offsetof(type, member)))

static void visit(
    const ttx_layout* base,
    ttx_abstract_callable* visitor) {
  const ttx_ranged_layout* self =
      TTX_CONTAINER_OF(base, const ttx_ranged_layout, layout);
  perimortem_count index;
  for (index = 0; index < self->count; ++index) {
    ttx_abstract_callable_call(visitor, self->entry);
  }
}

static perimortem_bool fits(
    const ttx_layout* self,
    const ttx_layout* target) {
  return ttx_layout_entries_fit(self, target);
}

static const ttx_layout_operations operations = {
    .visit = visit,
    .fits = fits,
};

void ttx_ranged_layout_initialize(
    ttx_ranged_layout* layout,
    const ttx_abstract* entry,
    perimortem_count count) {
  layout->layout.operations = &operations;
  layout->layout.named = 0;
  layout->entry = entry;
  layout->count = count;
}
