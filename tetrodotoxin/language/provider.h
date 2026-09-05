// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#ifndef TETRODOTOXIN_LANGUAGE_PROVIDER_H
#define TETRODOTOXIN_LANGUAGE_PROVIDER_H

#include "tetrodotoxin/language/production.h"
#include "ttx/abi.h"

typedef struct tetrodotoxin_dialect_provider_self
    tetrodotoxin_dialect_provider_self;
typedef struct tetrodotoxin_dialect_provider_ops
    tetrodotoxin_dialect_provider_ops;
typedef struct tetrodotoxin_source_input_self tetrodotoxin_source_input_self;
typedef struct tetrodotoxin_source_input_ops tetrodotoxin_source_input_ops;
typedef struct tetrodotoxin_source_graph_self tetrodotoxin_source_graph_self;
typedef struct tetrodotoxin_source_graph_ops tetrodotoxin_source_graph_ops;
typedef struct tetrodotoxin_interpret_result_self
    tetrodotoxin_interpret_result_self;
typedef struct tetrodotoxin_interpret_result_ops
    tetrodotoxin_interpret_result_ops;
typedef struct tetrodotoxin_validation_result_self
    tetrodotoxin_validation_result_self;
typedef struct tetrodotoxin_validation_result_ops
    tetrodotoxin_validation_result_ops;
typedef struct tetrodotoxin_source_dependency_self
    tetrodotoxin_source_dependency_self;
typedef struct tetrodotoxin_source_dependency_ops
    tetrodotoxin_source_dependency_ops;
typedef struct tetrodotoxin_source_dependency_sink_self
    tetrodotoxin_source_dependency_sink_self;
typedef struct tetrodotoxin_source_dependency_sink_ops
    tetrodotoxin_source_dependency_sink_ops;
typedef struct tetrodotoxin_dependency_result_self
    tetrodotoxin_dependency_result_self;
typedef struct tetrodotoxin_dependency_result_ops
    tetrodotoxin_dependency_result_ops;
typedef struct tetrodotoxin_token_sink_self tetrodotoxin_token_sink_self;
typedef struct tetrodotoxin_token_sink_ops tetrodotoxin_token_sink_ops;

typedef struct {
  const tetrodotoxin_dialect_provider_ops* operations;
  tetrodotoxin_dialect_provider_self* self;
} tetrodotoxin_dialect_provider;

typedef struct {
  const tetrodotoxin_source_input_ops* operations;
  tetrodotoxin_source_input_self* self;
} tetrodotoxin_source_input;

typedef struct {
  const tetrodotoxin_source_graph_ops* operations;
  tetrodotoxin_source_graph_self* self;
} tetrodotoxin_source_graph;

typedef struct {
  const tetrodotoxin_interpret_result_ops* operations;
  tetrodotoxin_interpret_result_self* self;
} tetrodotoxin_interpret_result;

typedef struct {
  const tetrodotoxin_validation_result_ops* operations;
  tetrodotoxin_validation_result_self* self;
} tetrodotoxin_validation_result;

typedef struct {
  const tetrodotoxin_source_dependency_ops* operations;
  tetrodotoxin_source_dependency_self* self;
} tetrodotoxin_source_dependency;

typedef struct {
  const tetrodotoxin_source_dependency_sink_ops* operations;
  tetrodotoxin_source_dependency_sink_self* self;
} tetrodotoxin_source_dependency_sink;

typedef struct {
  const tetrodotoxin_dependency_result_ops* operations;
  tetrodotoxin_dependency_result_self* self;
} tetrodotoxin_dependency_result;

typedef struct {
  const tetrodotoxin_token_sink_ops* operations;
  tetrodotoxin_token_sink_self* self;
} tetrodotoxin_token_sink;

typedef struct {
  uint64_t offset;
  uint64_t line;
  uint64_t column;
  uint64_t size;
  uint8_t code;
} tetrodotoxin_token;

typedef enum {
  TETRODOTOXIN_DEPENDENCY_SOURCE = 1,
  TETRODOTOXIN_DEPENDENCY_PACKAGE = 2,
} tetrodotoxin_dependency_kind;

// SourceInput exposes one immutable source snapshot. A frontend may consume
// the host's lexical projection or classify the raw bytes itself, keeping the
// shared TTX lexer useful without making it mandatory for another language.
// Source byte views remain valid until the returned SourceGraph is released;
// the input handle and token sink remain borrowed only for each operation.
struct tetrodotoxin_source_input_ops {
  ttx_abi_header header;
  ttx_borrowed_bytes(TTX_CALL* diagnostic_path)(
      tetrodotoxin_source_input_self*);
  ttx_borrowed_bytes(TTX_CALL* bytes)(tetrodotoxin_source_input_self*);
  void(TTX_CALL* visit_tokens)(
      tetrodotoxin_source_input_self*,
      tetrodotoxin_token_sink);
};

struct tetrodotoxin_token_sink_ops {
  ttx_abi_header header;
  void(TTX_CALL* token)(tetrodotoxin_token_sink_self*, tetrodotoxin_token);
  void(TTX_CALL* completed)(tetrodotoxin_token_sink_self*);
};

// DialectProvider separates the semantic candidate used for Interface proof
// from the retained operation handle that interprets source. Private C++, Rust,
// Zig, or remote dispatch state stays behind that handle while every returned
// identity crosses the canonical TTX ABI.
struct tetrodotoxin_dialect_provider_ops {
  ttx_abi_header header;
  void(TTX_CALL* retain)(tetrodotoxin_dialect_provider_self*);
  void(TTX_CALL* release)(tetrodotoxin_dialect_provider_self*);
  ttx_abstract(TTX_CALL* candidate)(tetrodotoxin_dialect_provider_self*);
  ttx_borrowed_bytes(TTX_CALL* name)(tetrodotoxin_dialect_provider_self*);
  void(TTX_CALL* interpret)(
      tetrodotoxin_dialect_provider_self*,
      tetrodotoxin_source_input,
      ttx_abstract context,
      tetrodotoxin_interpret_result);
};

struct tetrodotoxin_interpret_result_ops {
  ttx_abi_header header;
  void(TTX_CALL* constructed)(
      tetrodotoxin_interpret_result_self*,
      tetrodotoxin_source_graph);
  void(TTX_CALL* failed)(
      tetrodotoxin_interpret_result_self*,
      ttx_abstract error);
};

// SourceGraph keeps one interpretation alive while tools observe its root,
// Environment acquires its dependencies, and the selected language answers a
// default production request. Dependency handles remain identity-free source
// support, allowing Environment to bind an acquisition without introducing
// another dependency registry into the graph.
struct tetrodotoxin_source_graph_ops {
  ttx_abi_header header;
  void(TTX_CALL* retain)(tetrodotoxin_source_graph_self*);
  void(TTX_CALL* release)(tetrodotoxin_source_graph_self*);
  ttx_abstract(TTX_CALL* root)(tetrodotoxin_source_graph_self*);
  ttx_abstract(TTX_CALL* dialect)(tetrodotoxin_source_graph_self*);
  void(TTX_CALL* visit_dependencies)(
      tetrodotoxin_source_graph_self*,
      tetrodotoxin_source_dependency_sink);
  void(TTX_CALL* validate)(
      tetrodotoxin_source_graph_self*,
      tetrodotoxin_validation_result);
  void(TTX_CALL* produce)(
      tetrodotoxin_source_graph_self*,
      ttx_context,
      tetrodotoxin_production_result);
};

struct tetrodotoxin_source_dependency_sink_ops {
  ttx_abi_header header;
  void(TTX_CALL* dependency)(
      tetrodotoxin_source_dependency_sink_self*,
      tetrodotoxin_source_dependency);
  void(TTX_CALL* completed)(tetrodotoxin_source_dependency_sink_self*);
  void(TTX_CALL* failed)(
      tetrodotoxin_source_dependency_sink_self*,
      ttx_abstract error);
};

struct tetrodotoxin_source_dependency_ops {
  ttx_abi_header header;
  tetrodotoxin_dependency_kind(TTX_CALL* kind)(
      tetrodotoxin_source_dependency_self*);
  ttx_borrowed_bytes(TTX_CALL* local_name)(
      tetrodotoxin_source_dependency_self*);
  ttx_borrowed_bytes(TTX_CALL* locator)(tetrodotoxin_source_dependency_self*);
  ttx_borrowed_bytes(TTX_CALL* version)(tetrodotoxin_source_dependency_self*);
  ttx_borrowed_bytes(TTX_CALL* route)(tetrodotoxin_source_dependency_self*);
  void(TTX_CALL* acquire)(
      tetrodotoxin_source_dependency_self*,
      ttx_abstract target,
      tetrodotoxin_dependency_result);
};

struct tetrodotoxin_dependency_result_ops {
  ttx_abi_header header;
  void(TTX_CALL* acquired)(tetrodotoxin_dependency_result_self*);
  void(TTX_CALL* rejected)(tetrodotoxin_dependency_result_self*);
  void(TTX_CALL* failed)(
      tetrodotoxin_dependency_result_self*,
      ttx_abstract error);
};

// Validation is repeatable and does not alter a completed graph. An incomplete
// result keeps the retained Monograph available to source tooling. Only an
// accepted result may enter immutable Terminal production.
struct tetrodotoxin_validation_result_ops {
  ttx_abi_header header;
  void(TTX_CALL* accepted)(tetrodotoxin_validation_result_self*);
  void(TTX_CALL* incomplete)(tetrodotoxin_validation_result_self*);
  void(TTX_CALL* failed)(
      tetrodotoxin_validation_result_self*,
      ttx_abstract error);
};

TTX_EXTERN_C TTX_EXPORT ttx_abstract TTX_CALL
    tetrodotoxin_dialect_requirement(void);

#endif
