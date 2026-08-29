// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#ifndef TTX_CONCEPT_CALLABLE_H
#define TTX_CONCEPT_CALLABLE_H

#include "perimortem/abi/c/core.h"

typedef struct ttx_abstract ttx_abstract;

typedef struct ttx_abstract_callable ttx_abstract_callable;
typedef struct ttx_abstract_callable_operations {
  void (*call)(ttx_abstract_callable* self, const ttx_abstract* abstract);
} ttx_abstract_callable_operations;

struct ttx_abstract_callable {
  const ttx_abstract_callable_operations* operations;
};

typedef struct ttx_named_abstract_callable ttx_named_abstract_callable;
typedef struct ttx_named_abstract_callable_operations {
  void (*call)(
      ttx_named_abstract_callable* self,
      perimortem_bytes name,
      const ttx_abstract* abstract);
} ttx_named_abstract_callable_operations;

struct ttx_named_abstract_callable {
  const ttx_named_abstract_callable_operations* operations;
};

typedef struct ttx_bytes_callable ttx_bytes_callable;
typedef struct ttx_bytes_callable_operations {
  void (*call)(ttx_bytes_callable* self, perimortem_bytes bytes);
} ttx_bytes_callable_operations;

struct ttx_bytes_callable {
  const ttx_bytes_callable_operations* operations;
};

static inline void ttx_abstract_callable_call(
    ttx_abstract_callable* callable,
    const ttx_abstract* abstract) {
  callable->operations->call(callable, abstract);
}

static inline void ttx_named_abstract_callable_call(
    ttx_named_abstract_callable* callable,
    perimortem_bytes name,
    const ttx_abstract* abstract) {
  callable->operations->call(callable, name, abstract);
}

static inline void ttx_bytes_callable_call(
    ttx_bytes_callable* callable,
    perimortem_bytes bytes) {
  callable->operations->call(callable, bytes);
}

#endif
