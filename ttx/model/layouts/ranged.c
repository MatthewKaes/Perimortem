// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "ttx/model/layouts/ranged.h"

#include <stddef.h>

#include "ttx/model/layouts/common_internal.h"

#define TTX_CONTAINER_OF(pointer, type, member) \
  ((type*)((uint8_t*)(pointer) - offsetof(type, member)))

static void visit(
    const struct ttx_layout* base,
    struct ttx_abstract_callable* visitor) {
  const struct ttx_ranged_layout* self =
      TTX_CONTAINER_OF(base, const struct ttx_ranged_layout, layout);
  perimortem_count index;
  for (index = 0; index < self->count; ++index) {
    ttx_abstract_callable_call(visitor, self->entry);
  }
}

static perimortem_bool fits(
    const struct ttx_layout* self,
    const struct ttx_layout* target) {
  return ttx_layout_entries_fit(self, target);
}

static const struct ttx_layout_operations operations = {
  .visit = visit,
  .fits = fits,
  .interface = ttx_layout_interface_rejected,
};

void ttx_ranged_layout_initialize(
    struct ttx_ranged_layout* layout,
    const struct ttx_abstract* entry,
    perimortem_count count) {
  layout->layout.operations = &operations;
  layout->entry = entry;
  layout->count = count;
}
