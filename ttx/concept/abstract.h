// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#ifndef TTX_CONCEPT_ABSTRACT_H
#define TTX_CONCEPT_ABSTRACT_H

#include "ttx/concept/callable.h"
#include "ttx/concept/documentation.h"
#include "ttx/concept/interface.h"

struct ttx_abstract_operations {
  struct perimortem_bytes (*name)(const struct ttx_abstract* self);
  const struct ttx_documentation* (*documentation)(
      const struct ttx_abstract* self);
  const struct ttx_abstract* (*resolve)(const struct ttx_abstract* self);
  const struct ttx_abstract* (*resolve_concept)(
      const struct ttx_abstract* self,
      struct perimortem_bytes name);
  void (*visit_concepts)(
      const struct ttx_abstract* self,
      struct ttx_named_abstract_callable* visitor);
  const struct ttx_abstract* (*type)(const struct ttx_abstract* self);
  struct ttx_interface (*interface)(
      const struct ttx_abstract* self,
      const struct ttx_abstract* requirement);
};

struct ttx_abstract {
  const struct ttx_abstract_operations* operations;
};

PERIMORTEM_EXTERN_C struct perimortem_bytes ttx_abstract_name(
    const struct ttx_abstract* abstract);
PERIMORTEM_EXTERN_C const struct ttx_documentation*
    ttx_abstract_documentation(const struct ttx_abstract* abstract);
PERIMORTEM_EXTERN_C const struct ttx_abstract* ttx_abstract_resolve(
    const struct ttx_abstract* abstract);
PERIMORTEM_EXTERN_C const struct ttx_abstract* ttx_abstract_type(
    const struct ttx_abstract* abstract);
PERIMORTEM_EXTERN_C const struct ttx_abstract* ttx_abstract_resolve_concept(
    const struct ttx_abstract* abstract,
    struct perimortem_bytes name);
PERIMORTEM_EXTERN_C void ttx_abstract_visit_concepts(
    const struct ttx_abstract* abstract,
    struct ttx_named_abstract_callable* visitor);
PERIMORTEM_EXTERN_C struct ttx_interface ttx_abstract_interface(
    const struct ttx_abstract* abstract,
    const struct ttx_abstract* requirement);

PERIMORTEM_EXTERN_C const struct ttx_abstract* ttx_abstract_requirement(void);
PERIMORTEM_EXTERN_C const struct ttx_abstract* ttx_unknown(void);
PERIMORTEM_EXTERN_C const struct ttx_abstract* ttx_none(void);

#endif
