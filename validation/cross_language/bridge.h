// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#ifndef VALIDATION_CROSS_LANGUAGE_BRIDGE_H
#define VALIDATION_CROSS_LANGUAGE_BRIDGE_H

#include "ttx/abi.h"

typedef struct {
  ttx_abstract echo;
  ttx_abstract view_bytes;
  ttx_abstract value;
} ttx_test_rust_exports;

typedef struct {
  ttx_abstract candidate;
  ttx_abstract requirement;
  ttx_abstract operation;
} ttx_test_policy_exports;

typedef enum {
  TTX_TEST_CONSUMED_UNKNOWN = 0,
  TTX_TEST_CONSUMED_REJECTED = 1,
  TTX_TEST_CONSUMED_SATISFIED = 2,
  TTX_TEST_CONSUMED_EQUIVALENT = 3,
  TTX_TEST_CONSUMED_SUPPORT_FAILED = 4
} ttx_test_consumption;

typedef struct {
  ttx_test_consumption outcome;
  uint64_t cardinality;
  ttx_pack arguments;
  ttx_pack result;
} ttx_test_receipt;

typedef struct {
  uint64_t visited;
  uint64_t unknown;
  uint64_t rejected;
  uint64_t satisfied;
  uint64_t equivalent;
  uint64_t support_failed;
} ttx_test_discovery;

typedef struct ttx_test_bytes_terminal_ops ttx_test_bytes_terminal_ops;
typedef struct ttx_test_bytes_sink_ops ttx_test_bytes_sink_ops;
typedef struct ttx_test_bytes_terminal_self ttx_test_bytes_terminal_self;
typedef struct ttx_test_bytes_sink_self ttx_test_bytes_sink_self;

typedef struct {
  const ttx_test_bytes_terminal_ops* operations;
  ttx_test_bytes_terminal_self* self;
} ttx_test_bytes_terminal;

typedef struct {
  const ttx_test_bytes_sink_ops* operations;
  ttx_test_bytes_sink_self* self;
} ttx_test_bytes_sink;

struct ttx_test_bytes_terminal_ops {
  ttx_abi_header header;
  void(TTX_CALL* project)(
      ttx_test_bytes_terminal,
      ttx_abstract producer,
      ttx_abstract requirement,
      ttx_test_bytes_sink);
};

struct ttx_test_bytes_sink_ops {
  ttx_abi_header header;
  void(TTX_CALL* rejected)(ttx_test_bytes_sink);
  void(TTX_CALL* projected)(ttx_test_bytes_sink, ttx_borrowed_bytes);
};

// Rust owns both requirements and the original candidate.
TTX_EXTERN_C ttx_test_rust_exports TTX_CALL
    rust_test_exports(ttx_test_bytes_terminal bytes_terminal);
TTX_EXTERN_C ttx_abstract TTX_CALL rust_echo_create(ttx_abstract value);
TTX_EXTERN_C ttx_test_policy_exports TTX_CALL rust_policy_exports(void);
TTX_EXTERN_C void TTX_CALL
    rust_temporary_pack(ttx_abstract, ttx_context, ttx_pack_result);

// C owns support storage and consumes an arbitrary candidate/requirement pair.
// Its implementation contains no knowledge of the Rust domains.
TTX_EXTERN_C ttx_test_bytes_terminal TTX_CALL
    ttx_test_create_bytes_terminal(void);
TTX_EXTERN_C ttx_abstract TTX_CALL ttx_test_c_integer_create(
    ttx_abstract value_requirement,
    ttx_abstract view_bytes_requirement,
    ttx_abstract to_string_operation);
TTX_EXTERN_C ttx_abstract TTX_CALL ttx_test_c_boolean_create(
    ttx_abstract value_requirement,
    ttx_abstract view_bytes_requirement,
    ttx_abstract to_string_operation);
TTX_EXTERN_C ttx_addressable_policy TTX_CALL
    ttx_test_c_restriction_policy(void);
TTX_EXTERN_C ttx_test_receipt TTX_CALL ttx_test_consume_pack(
    ttx_abstract candidate,
    ttx_abstract requirement,
    ttx_abstract operation,
    ttx_pack input,
    ttx_context context);
TTX_EXTERN_C ttx_test_receipt TTX_CALL ttx_test_consume_empty(
    ttx_abstract candidate,
    ttx_abstract requirement,
    ttx_abstract operation,
    ttx_context context);
TTX_EXTERN_C ttx_test_discovery TTX_CALL ttx_test_discover_empty(
    ttx_abstract root,
    ttx_abstract requirement,
    ttx_abstract operation,
    ttx_context context);
TTX_EXTERN_C uint64_t TTX_CALL ttx_test_pack_cardinality(ttx_pack pack);
TTX_EXTERN_C ttx_abstract TTX_CALL ttx_test_pack_first(ttx_pack pack);

#endif
