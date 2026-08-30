// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "ttx/model/documentations/merged.h"

#include <stddef.h>

#define TTX_CONTAINER_OF(pointer, type, member) \
  ((type*)((uint8_t*)(pointer)-offsetof(type, member)))

static void visit(
    const struct ttx_documentation* base,
    struct ttx_bytes_callable* visitor) {
  const struct ttx_merged_documentation* self =
      TTX_CONTAINER_OF(base, const struct ttx_merged_documentation, documentation);
  ttx_documentation_visit(self->first, visitor);
  ttx_documentation_visit(self->second, visitor);
}

static const struct ttx_documentation_operations operations = {
    .visit = visit,
};

void ttx_merged_documentation_initialize(
    struct ttx_merged_documentation* documentation,
    const struct ttx_documentation* first,
    const struct ttx_documentation* second) {
  documentation->documentation.operations = &operations;
  documentation->first = first;
  documentation->second = second;
}
