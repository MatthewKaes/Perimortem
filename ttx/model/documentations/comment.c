// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "ttx/model/documentations/comment.h"

#include <stddef.h>

#define TTX_CONTAINER_OF(pointer, type, member) \
  ((type*)((uint8_t*)(pointer)-offsetof(type, member)))

static void visit(
    const struct ttx_documentation* base,
    struct ttx_bytes_callable* visitor) {
  const struct ttx_comment_documentation* self =
      TTX_CONTAINER_OF(base, const struct ttx_comment_documentation, documentation);
  if (self->line.size != 0) {
    ttx_bytes_callable_call(visitor, self->line);
  }
}

static const struct ttx_documentation_operations operations = {
    .visit = visit,
};

void ttx_comment_documentation_initialize(
    struct ttx_comment_documentation* documentation,
    struct perimortem_bytes line) {
  documentation->documentation.operations = &operations;
  documentation->line = line;
}
