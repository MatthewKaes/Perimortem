// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#ifndef TTX_CONCEPT_CALLABLE_H
#define TTX_CONCEPT_CALLABLE_H

#include "perimortem/core/perimortem.h"

struct ttx_abstract;

// Stateful visitors use a typed Callable handle instead of the usual function
// and void pointer pair found in C callback APIs. The operation receives that
// same handle as self, which keeps captured state recoverable without erased
// context and maps directly to a self hosted Callable later.
//
// Visit operations are synchronous and retain no Callable after returning.
// Each handle therefore needs to remain valid only for the call that receives
// it.
struct ttx_abstract_callable;
struct ttx_abstract_callable_operations {
  void (*call)(
      struct ttx_abstract_callable* self,
      const struct ttx_abstract* abstract);
};

struct ttx_abstract_callable {
  const struct ttx_abstract_callable_operations* operations;
};

// Concept and named Layout visitation share this signature because both expose
// one borrowed binary name beside one exact Abstract identity.
struct ttx_named_abstract_callable;
struct ttx_named_abstract_callable_operations {
  void (*call)(
      struct ttx_named_abstract_callable* self,
      struct perimortem_bytes name,
      const struct ttx_abstract* abstract);
};

struct ttx_named_abstract_callable {
  const struct ttx_named_abstract_callable_operations* operations;
};

// Byte visitation keeps serialization and presentation consumers on the same
// typed callback model without giving raw bytes semantic identity.
struct ttx_bytes_callable;
struct ttx_bytes_callable_operations {
  void (*call)(struct ttx_bytes_callable* self, struct perimortem_bytes bytes);
};

struct ttx_bytes_callable {
  const struct ttx_bytes_callable_operations* operations;
};

static inline void ttx_abstract_callable_call(
    struct ttx_abstract_callable* callable,
    const struct ttx_abstract* abstract) {
  callable->operations->call(callable, abstract);
}

static inline void ttx_named_abstract_callable_call(
    struct ttx_named_abstract_callable* callable,
    struct perimortem_bytes name,
    const struct ttx_abstract* abstract) {
  callable->operations->call(callable, name, abstract);
}

static inline void ttx_bytes_callable_call(
    struct ttx_bytes_callable* callable,
    struct perimortem_bytes bytes) {
  callable->operations->call(callable, bytes);
}

#endif
