// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#ifndef TETRODOTOXIN_TERMINAL_PROVIDER_H
#define TETRODOTOXIN_TERMINAL_PROVIDER_H

#include "ttx/abi.h"

typedef struct tetrodotoxin_workspace_view_self
    tetrodotoxin_workspace_view_self;
typedef struct tetrodotoxin_workspace_view_ops tetrodotoxin_workspace_view_ops;
typedef struct {
  const tetrodotoxin_workspace_view_ops* operations;
  tetrodotoxin_workspace_view_self* self;
} tetrodotoxin_workspace_view;

typedef struct tetrodotoxin_terminal_provider_self
    tetrodotoxin_terminal_provider_self;
typedef struct tetrodotoxin_terminal_provider_ops
    tetrodotoxin_terminal_provider_ops;
typedef struct tetrodotoxin_product_request_self
    tetrodotoxin_product_request_self;
typedef struct tetrodotoxin_product_request_ops
    tetrodotoxin_product_request_ops;
typedef struct tetrodotoxin_production_closure_self
    tetrodotoxin_production_closure_self;
typedef struct tetrodotoxin_production_closure_ops
    tetrodotoxin_production_closure_ops;
typedef struct tetrodotoxin_terminal_begin_result_self
    tetrodotoxin_terminal_begin_result_self;
typedef struct tetrodotoxin_terminal_begin_result_ops
    tetrodotoxin_terminal_begin_result_ops;
typedef struct tetrodotoxin_product_observation_result_self
    tetrodotoxin_product_observation_result_self;
typedef struct tetrodotoxin_product_observation_result_ops
    tetrodotoxin_product_observation_result_ops;
typedef struct tetrodotoxin_product_cancel_result_self
    tetrodotoxin_product_cancel_result_self;
typedef struct tetrodotoxin_product_cancel_result_ops
    tetrodotoxin_product_cancel_result_ops;
typedef struct tetrodotoxin_closure_constant_sink_self
    tetrodotoxin_closure_constant_sink_self;
typedef struct tetrodotoxin_closure_constant_sink_ops
    tetrodotoxin_closure_constant_sink_ops;
typedef struct tetrodotoxin_reobserve_self tetrodotoxin_reobserve_self;
typedef struct tetrodotoxin_reobserve_ops tetrodotoxin_reobserve_ops;
typedef struct tetrodotoxin_reobserve_callback_self
    tetrodotoxin_reobserve_callback_self;
typedef struct tetrodotoxin_reobserve_callback_ops
    tetrodotoxin_reobserve_callback_ops;
typedef struct tetrodotoxin_reobserve_prepare_result_self
    tetrodotoxin_reobserve_prepare_result_self;
typedef struct tetrodotoxin_reobserve_prepare_result_ops
    tetrodotoxin_reobserve_prepare_result_ops;
typedef struct tetrodotoxin_reobserve_commit_result_self
    tetrodotoxin_reobserve_commit_result_self;
typedef struct tetrodotoxin_reobserve_commit_result_ops
    tetrodotoxin_reobserve_commit_result_ops;
typedef struct tetrodotoxin_reobserve_close_result_self
    tetrodotoxin_reobserve_close_result_self;
typedef struct tetrodotoxin_reobserve_close_result_ops
    tetrodotoxin_reobserve_close_result_ops;
typedef struct tetrodotoxin_authority_revision_self
    tetrodotoxin_authority_revision_self;
typedef struct tetrodotoxin_authority_revision_ops
    tetrodotoxin_authority_revision_ops;
typedef struct tetrodotoxin_closure_authority_sink_self
    tetrodotoxin_closure_authority_sink_self;
typedef struct tetrodotoxin_closure_authority_sink_ops
    tetrodotoxin_closure_authority_sink_ops;

typedef struct {
  const tetrodotoxin_terminal_provider_ops* operations;
  tetrodotoxin_terminal_provider_self* self;
} tetrodotoxin_terminal_provider;

typedef struct {
  const tetrodotoxin_product_request_ops* operations;
  tetrodotoxin_product_request_self* self;
} tetrodotoxin_product_request;

typedef struct {
  const tetrodotoxin_production_closure_ops* operations;
  tetrodotoxin_production_closure_self* self;
} tetrodotoxin_production_closure;

typedef struct {
  const tetrodotoxin_terminal_begin_result_ops* operations;
  tetrodotoxin_terminal_begin_result_self* self;
} tetrodotoxin_terminal_begin_result;

typedef struct {
  const tetrodotoxin_product_observation_result_ops* operations;
  tetrodotoxin_product_observation_result_self* self;
} tetrodotoxin_product_observation_result;

typedef struct {
  const tetrodotoxin_product_cancel_result_ops* operations;
  tetrodotoxin_product_cancel_result_self* self;
} tetrodotoxin_product_cancel_result;

typedef struct {
  const tetrodotoxin_closure_constant_sink_ops* operations;
  tetrodotoxin_closure_constant_sink_self* self;
} tetrodotoxin_closure_constant_sink;

typedef struct {
  const tetrodotoxin_reobserve_ops* operations;
  tetrodotoxin_reobserve_self* self;
} tetrodotoxin_reobserve;

typedef struct {
  const tetrodotoxin_reobserve_callback_ops* operations;
  tetrodotoxin_reobserve_callback_self* self;
} tetrodotoxin_reobserve_callback;

typedef struct {
  const tetrodotoxin_reobserve_prepare_result_ops* operations;
  tetrodotoxin_reobserve_prepare_result_self* self;
} tetrodotoxin_reobserve_prepare_result;

typedef struct {
  const tetrodotoxin_reobserve_commit_result_ops* operations;
  tetrodotoxin_reobserve_commit_result_self* self;
} tetrodotoxin_reobserve_commit_result;

typedef struct {
  const tetrodotoxin_reobserve_close_result_ops* operations;
  tetrodotoxin_reobserve_close_result_self* self;
} tetrodotoxin_reobserve_close_result;

typedef struct {
  const tetrodotoxin_authority_revision_ops* operations;
  tetrodotoxin_authority_revision_self* self;
} tetrodotoxin_authority_revision;

typedef struct {
  const tetrodotoxin_closure_authority_sink_ops* operations;
  tetrodotoxin_closure_authority_sink_self* self;
} tetrodotoxin_closure_authority_sink;

// A Terminal provider has one semantic candidate used for Interface proof and
// one retained operation surface used after that proof succeeds. Keeping both
// on this typed handle lets a plugin implement production in C, C++, Rust, or
// another language without turning its native object address into identity.
struct tetrodotoxin_terminal_provider_ops {
  ttx_abi_header header;
  void(TTX_CALL* retain)(tetrodotoxin_terminal_provider_self*);
  void(TTX_CALL* release)(tetrodotoxin_terminal_provider_self*);
  ttx_abstract(TTX_CALL* candidate)(tetrodotoxin_terminal_provider_self*);
  void(TTX_CALL* begin)(
      tetrodotoxin_terminal_provider_self*,
      ttx_borrowed_bytes output_route,
      ttx_abstract product,
      tetrodotoxin_workspace_view workspace,
      ttx_abstract environment,
      ttx_abstract invocation,
      tetrodotoxin_terminal_begin_result);
};

struct tetrodotoxin_terminal_begin_result_ops {
  ttx_abi_header header;
  void(TTX_CALL* requested)(
      tetrodotoxin_terminal_begin_result_self*,
      tetrodotoxin_product_request);
  void(TTX_CALL* failed)(
      tetrodotoxin_terminal_begin_result_self*,
      ttx_abstract error);
};

// A request keeps every owner needed by its output Pack alive. observe returns
// exactly one branch before returning and never retains the result sink. A
// successful request remains retained through publication because its Pack and
// sealed dependency closure borrow request-owned support.
struct tetrodotoxin_product_request_ops {
  ttx_abi_header header;
  void(TTX_CALL* retain)(tetrodotoxin_product_request_self*);
  void(TTX_CALL* release)(tetrodotoxin_product_request_self*);
  void(TTX_CALL* observe)(
      tetrodotoxin_product_request_self*,
      tetrodotoxin_product_observation_result);
  void(TTX_CALL* cancel)(
      tetrodotoxin_product_request_self*,
      tetrodotoxin_product_cancel_result);
  void(TTX_CALL* prepare_reobserve)(
      tetrodotoxin_product_request_self*,
      tetrodotoxin_reobserve_callback,
      tetrodotoxin_reobserve_prepare_result);
};

#define TETRODOTOXIN_PRODUCT_REQUEST_PREFIX_SIZE \
  offsetof(tetrodotoxin_product_request_ops, prepare_reobserve)

// Reobserve closes the race between an Unknown observation and a provider
// change. prepare creates a disarmed one-shot arm, commit reports whether the
// change already raced or makes one later callback possible, and close returns
// only after no callback can still enter caller storage.
struct tetrodotoxin_reobserve_callback_ops {
  ttx_abi_header header;
  void(TTX_CALL* changed)(tetrodotoxin_reobserve_callback_self*);
};

struct tetrodotoxin_reobserve_prepare_result_ops {
  ttx_abi_header header;
  void(TTX_CALL* prepared)(
      tetrodotoxin_reobserve_prepare_result_self*,
      tetrodotoxin_reobserve);
  void(TTX_CALL* failed)(
      tetrodotoxin_reobserve_prepare_result_self*,
      ttx_abstract error);
};

struct tetrodotoxin_reobserve_ops {
  ttx_abi_header header;
  void(TTX_CALL* commit)(
      tetrodotoxin_reobserve_self*,
      tetrodotoxin_reobserve_commit_result);
  void(TTX_CALL* close)(
      tetrodotoxin_reobserve_self*,
      tetrodotoxin_reobserve_close_result);
};

struct tetrodotoxin_reobserve_commit_result_ops {
  ttx_abi_header header;
  void(TTX_CALL* changed)(tetrodotoxin_reobserve_commit_result_self*);
  void(TTX_CALL* armed)(tetrodotoxin_reobserve_commit_result_self*);
  void(TTX_CALL* failed)(
      tetrodotoxin_reobserve_commit_result_self*,
      ttx_abstract error);
};

struct tetrodotoxin_reobserve_close_result_ops {
  ttx_abi_header header;
  void(TTX_CALL* closed)(tetrodotoxin_reobserve_close_result_self*);
  void(TTX_CALL* failed)(
      tetrodotoxin_reobserve_close_result_self*,
      ttx_abstract error);
};

struct tetrodotoxin_product_observation_result_ops {
  ttx_abi_header header;
  void(TTX_CALL* unknown)(tetrodotoxin_product_observation_result_self*);
  void(TTX_CALL* none)(tetrodotoxin_product_observation_result_self*);
  void(TTX_CALL* failed)(
      tetrodotoxin_product_observation_result_self*,
      ttx_abstract error);
  void(TTX_CALL* produced)(
      tetrodotoxin_product_observation_result_self*,
      ttx_pack products,
      tetrodotoxin_production_closure closure);
};

struct tetrodotoxin_product_cancel_result_ops {
  ttx_abi_header header;
  void(TTX_CALL* cancelled)(tetrodotoxin_product_cancel_result_self*);
  void(TTX_CALL* already_settled)(tetrodotoxin_product_cancel_result_self*);
  void(TTX_CALL* failed)(
      tetrodotoxin_product_cancel_result_self*,
      ttx_abstract error);
};

// ProductionClosure records the immutable inputs consumed by one request.
// This first prefix exposes exact Constants; later ABI tails can add authority
// revisions and wake coordination without changing a completed request's
// existing observation contract.
struct tetrodotoxin_production_closure_ops {
  ttx_abi_header header;
  void(TTX_CALL* visit_constants)(
      tetrodotoxin_production_closure_self*,
      tetrodotoxin_closure_constant_sink);
  void(TTX_CALL* visit_authorities)(
      tetrodotoxin_production_closure_self*,
      tetrodotoxin_closure_authority_sink);
};

#define TETRODOTOXIN_PRODUCTION_CLOSURE_PREFIX_SIZE \
  offsetof(tetrodotoxin_production_closure_ops, visit_authorities)

struct tetrodotoxin_closure_constant_sink_ops {
  ttx_abi_header header;
  void(TTX_CALL* constant)(
      tetrodotoxin_closure_constant_sink_self*,
      ttx_abstract constant);
  void(TTX_CALL* completed)(tetrodotoxin_closure_constant_sink_self*);
};

// Authority revisions are request-owned support records rather than semantic
// identities. A bridge assigns stable owner and value tokens to one mutable
// endpoint, while revision changes whenever that endpoint replaces the answers
// a request may observe.
struct tetrodotoxin_authority_revision_ops {
  ttx_abi_header header;
  uint64_t(TTX_CALL* revision)(tetrodotoxin_authority_revision_self*);
  uint8_t(TTX_CALL* is_current)(tetrodotoxin_authority_revision_self*);
};

struct tetrodotoxin_closure_authority_sink_ops {
  ttx_abi_header header;
  void(TTX_CALL* authority)(
      tetrodotoxin_closure_authority_sink_self*,
      tetrodotoxin_authority_revision);
  void(TTX_CALL* completed)(tetrodotoxin_closure_authority_sink_self*);
};

TTX_EXTERN_C TTX_EXPORT ttx_abstract TTX_CALL
    tetrodotoxin_terminal_requirement(void);

#endif
