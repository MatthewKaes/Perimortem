// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "ttx/concept/documentation.h"

static void visit_empty(
    const struct ttx_documentation* self,
    struct ttx_bytes_callable* visitor) {
  (void)self;
  (void)visitor;
}

static const struct ttx_documentation_operations empty_operations = {
    .visit = visit_empty,
};

static const struct ttx_documentation empty_documentation = {
    .operations = &empty_operations,
};

void ttx_documentation_visit(
    const struct ttx_documentation* documentation,
    struct ttx_bytes_callable* visitor) {
  documentation->operations->visit(documentation, visitor);
}

const struct ttx_documentation* ttx_documentation_empty(void) {
  return &empty_documentation;
}
