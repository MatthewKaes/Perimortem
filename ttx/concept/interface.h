// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#ifndef TTX_CONCEPT_INTERFACE_H
#define TTX_CONCEPT_INTERFACE_H

#include "perimortem/abi/c/core.h"

typedef struct ttx_abstract ttx_abstract;

typedef enum ttx_interface_relation {
  TTX_INTERFACE_REJECTED = 0,
  TTX_INTERFACE_SATISFIED = 1,
  TTX_INTERFACE_EQUIVALENT = 2,
} ttx_interface_relation;

typedef struct ttx_interface_operations {
  ttx_interface_relation (*negotiate)(
      const ttx_abstract* requirement,
      const ttx_abstract* candidate);
} ttx_interface_operations;

typedef struct ttx_interface {
  const ttx_abstract* requirement;
  const ttx_abstract* candidate;
  const ttx_interface_operations* operations;
} ttx_interface;

PERIMORTEM_EXTERN_C ttx_interface_relation ttx_interface_negotiate(
    const ttx_interface* interface);

PERIMORTEM_EXTERN_C perimortem_bool ttx_interface_accepts(const ttx_interface* interface);

PERIMORTEM_EXTERN_C ttx_interface ttx_interface_rejected(
    const ttx_abstract* requirement,
    const ttx_abstract* candidate);

PERIMORTEM_EXTERN_C ttx_interface ttx_interface_satisfied(
    const ttx_abstract* requirement,
    const ttx_abstract* candidate,
    const ttx_interface_operations* operations);

PERIMORTEM_EXTERN_C const ttx_interface_operations* ttx_interface_marker_operations(void);

#endif
