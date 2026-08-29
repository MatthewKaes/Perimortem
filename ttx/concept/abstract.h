// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#ifndef TTX_CONCEPT_ABSTRACT_H
#define TTX_CONCEPT_ABSTRACT_H

#include "ttx/concept/callable.h"
#include "ttx/concept/documentation.h"
#include "ttx/concept/interface.h"

typedef struct ttx_abstract_operations {
  perimortem_bytes (*name)(const ttx_abstract* self);
  const ttx_documentation* (*documentation)(const ttx_abstract* self);
  const ttx_abstract* (*resolve)(const ttx_abstract* self);
  const ttx_abstract* (*type)(const ttx_abstract* self);
  const ttx_abstract* (*resolve_concept)(
      const ttx_abstract* self,
      perimortem_bytes name);
  void (*visit_concepts)(
      const ttx_abstract* self,
      ttx_named_abstract_callable* visitor);
  ttx_interface (*interface)(
      const ttx_abstract* self,
      const ttx_abstract* requirement);
} ttx_abstract_operations;

struct ttx_abstract {
  const ttx_abstract_operations* operations;
};

PERIMORTEM_EXTERN_C perimortem_bytes ttx_abstract_name(const ttx_abstract* abstract);
PERIMORTEM_EXTERN_C const ttx_documentation* ttx_abstract_documentation(
    const ttx_abstract* abstract);
PERIMORTEM_EXTERN_C const ttx_abstract* ttx_abstract_resolve(const ttx_abstract* abstract);
PERIMORTEM_EXTERN_C const ttx_abstract* ttx_abstract_type(const ttx_abstract* abstract);
PERIMORTEM_EXTERN_C const ttx_abstract* ttx_abstract_resolve_concept(
    const ttx_abstract* abstract,
    perimortem_bytes name);
PERIMORTEM_EXTERN_C void ttx_abstract_visit_concepts(
    const ttx_abstract* abstract,
    ttx_named_abstract_callable* visitor);
PERIMORTEM_EXTERN_C ttx_interface ttx_abstract_interface(
    const ttx_abstract* abstract,
    const ttx_abstract* requirement);

PERIMORTEM_EXTERN_C const ttx_abstract* ttx_abstract_requirement(void);
PERIMORTEM_EXTERN_C const ttx_abstract* ttx_unknown(void);
PERIMORTEM_EXTERN_C const ttx_abstract* ttx_none(void);

#endif
