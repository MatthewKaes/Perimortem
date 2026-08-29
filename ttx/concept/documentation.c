// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "ttx/concept/documentation.h"

static void visit_empty(
    const ttx_documentation* self,
    ttx_bytes_callable* visitor) {
  (void)self;
  (void)visitor;
}

static const ttx_documentation_operations empty_operations = {
    .visit = visit_empty,
};

static const ttx_documentation empty_documentation = {
    .operations = &empty_operations,
};

void ttx_documentation_visit(
    const ttx_documentation* documentation,
    ttx_bytes_callable* visitor) {
  documentation->operations->visit(documentation, visitor);
}

const ttx_documentation* ttx_documentation_empty(void) {
  return &empty_documentation;
}
