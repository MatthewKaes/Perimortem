// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#ifndef TETRODOTOXIN_LANGUAGE_PRODUCTION_H
#define TETRODOTOXIN_LANGUAGE_PRODUCTION_H

#include "tetrodotoxin/terminal/provider.h"
#include "ttx/abi.h"

typedef struct tetrodotoxin_source_production_self
    tetrodotoxin_source_production_self;
typedef struct tetrodotoxin_source_production_ops
    tetrodotoxin_source_production_ops;
typedef struct tetrodotoxin_product_request_sink_self
    tetrodotoxin_product_request_sink_self;
typedef struct tetrodotoxin_product_request_sink_ops
    tetrodotoxin_product_request_sink_ops;
typedef struct tetrodotoxin_production_result_self
    tetrodotoxin_production_result_self;
typedef struct tetrodotoxin_production_result_ops
    tetrodotoxin_production_result_ops;

typedef struct {
  const tetrodotoxin_source_production_ops* operations;
  tetrodotoxin_source_production_self* self;
} tetrodotoxin_source_production;

typedef struct {
  const tetrodotoxin_product_request_sink_ops* operations;
  tetrodotoxin_product_request_sink_self* self;
} tetrodotoxin_product_request_sink;

typedef struct {
  const tetrodotoxin_production_result_ops* operations;
  tetrodotoxin_production_result_self* self;
} tetrodotoxin_production_result;

// Source production resolves authored policy once, then transfers one retained
// plan to the invocation driver. The plan keeps its Environment, providers,
// and request operation tables alive; each ProductRequest continues to own the
// Context and output Pack produced by its Terminal.
struct tetrodotoxin_source_production_ops {
  ttx_abi_header header;
  void(TTX_CALL* retain)(tetrodotoxin_source_production_self*);
  void(TTX_CALL* release)(tetrodotoxin_source_production_self*);
  ttx_borrowed_bytes(TTX_CALL* publication_root)(
      tetrodotoxin_source_production_self*);
  void(TTX_CALL* visit_requests)(
      tetrodotoxin_source_production_self*,
      tetrodotoxin_product_request_sink);
};

struct tetrodotoxin_product_request_sink_ops {
  ttx_abi_header header;
  void(TTX_CALL* request)(
      tetrodotoxin_product_request_sink_self*,
      tetrodotoxin_product_request);
  void(TTX_CALL* completed)(tetrodotoxin_product_request_sink_self*);
  void(TTX_CALL* failed)(
      tetrodotoxin_product_request_sink_self*,
      ttx_abstract error);
};

// Unknown asks the caller to repeat source-level planning after its graph gains
// more information. None proves that this Dialect has no default production.
// A completed plan contains the independent Terminal requests selected by that
// source without turning them into graph identities or a copied product Pack.
struct tetrodotoxin_production_result_ops {
  ttx_abi_header header;
  void(TTX_CALL* unknown)(tetrodotoxin_production_result_self*);
  void(TTX_CALL* none)(tetrodotoxin_production_result_self*);
  void(TTX_CALL* failed)(
      tetrodotoxin_production_result_self*,
      ttx_abstract error);
  void(TTX_CALL* planned)(
      tetrodotoxin_production_result_self*,
      tetrodotoxin_source_production production);
};

#endif
