// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "ttx/model/documentations/block.h"

#include <stddef.h>

#define TTX_CONTAINER_OF(pointer, type, member) \
  ((type*)((uint8_t*)(pointer)-offsetof(type, member)))

static void visit(
    const struct ttx_documentation* base,
    struct ttx_bytes_callable* visitor) {
  const struct ttx_block_documentation* self =
      TTX_CONTAINER_OF(base, const struct ttx_block_documentation, documentation);
  perimortem_count index;
  for (index = 0; index < self->count; ++index) {
    ttx_bytes_callable_call(visitor, self->lines[index]);
  }
}

static const struct ttx_documentation_operations operations = {
    .visit = visit,
};

void ttx_block_documentation_initialize(
    struct ttx_block_documentation* documentation,
    const struct perimortem_bytes* lines,
    perimortem_count count) {
  documentation->documentation.operations = &operations;
  documentation->lines = lines;
  documentation->count = count;
}
