// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#ifndef TETRODOTOXIN_LANGUAGE_SOURCE_H
#define TETRODOTOXIN_LANGUAGE_SOURCE_H

#include "ttx/abi.h"

typedef struct tetrodotoxin_source_associations_self tetrodotoxin_source_associations_self;
typedef struct tetrodotoxin_source_diagnostics_self tetrodotoxin_source_diagnostics_self;
typedef struct tetrodotoxin_source_associations_ops tetrodotoxin_source_associations_ops;
typedef struct tetrodotoxin_source_diagnostics_ops tetrodotoxin_source_diagnostics_ops;

// Byte coordinates keep a foreign frontend's source locations usable without
// requiring its token representation. A zero focus size leaves only the span.
typedef struct {
  uint64_t offset;
  uint64_t size;
  uint64_t focus_offset;
  uint64_t focus_size;
} tetrodotoxin_source_anchor;

typedef struct {
  const tetrodotoxin_source_associations_ops* operations;
  tetrodotoxin_source_associations_self* self;
} tetrodotoxin_source_associations;

typedef struct {
  const tetrodotoxin_source_diagnostics_ops* operations;
  tetrodotoxin_source_diagnostics_self* self;
} tetrodotoxin_source_diagnostics;

// Both visits are synchronous. Associations borrow exact graph identities,
// while diagnostics lend presentation bytes only for their callback. Keeping
// the visits on the source provider lets an editor inspect another frontend
// without constructing a second symbol or diagnostic model inside Workspace.
struct tetrodotoxin_source_associations_ops {
  ttx_abi_header header;
  void(TTX_CALL* association)(tetrodotoxin_source_associations,
      tetrodotoxin_source_anchor, ttx_abstract);
  void(TTX_CALL* completed)(tetrodotoxin_source_associations);
};

struct tetrodotoxin_source_diagnostics_ops {
  ttx_abi_header header;
  void(TTX_CALL* diagnostic)(tetrodotoxin_source_diagnostics,
      tetrodotoxin_source_anchor, ttx_borrowed_bytes message);
  void(TTX_CALL* completed)(tetrodotoxin_source_diagnostics);
};

#endif
