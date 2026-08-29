// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#ifndef TETRODOTOXIN_LIBRARY_LANGUAGE_MODEL_FOLD_CALL_H
#define TETRODOTOXIN_LIBRARY_LANGUAGE_MODEL_FOLD_CALL_H

#include "ttx/concept/abstract.h"
#include "ttx/concept/pack.h"

typedef struct ttx_library_fold_call_operations {
  ttx_interface_operations interface;
  const ttx_abstract* (*fold)(
      const ttx_abstract* callable,
      const ttx_pack* receiver,
      const ttx_pack* arguments);
} ttx_library_fold_call_operations;

typedef struct ttx_library_fold_call_view {
  const ttx_abstract* identity;
  const ttx_library_fold_call_operations* operations;
} ttx_library_fold_call_view;

PERIMORTEM_EXTERN_C const ttx_abstract* ttx_library_fold_call_requirement(void);
PERIMORTEM_EXTERN_C perimortem_bool ttx_library_fold_call_prove(
    const ttx_abstract* candidate,
    ttx_library_fold_call_view* view);
PERIMORTEM_EXTERN_C const ttx_abstract* ttx_library_fold_call(
    const ttx_library_fold_call_view* callable,
    const ttx_pack* receiver,
    const ttx_pack* arguments);
PERIMORTEM_EXTERN_C ttx_interface_relation ttx_library_fold_call_relation(
    const ttx_abstract* requirement,
    const ttx_abstract* candidate);

#endif
