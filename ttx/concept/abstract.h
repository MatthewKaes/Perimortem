// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#ifndef TTX_CONCEPT_ABSTRACT_H
#define TTX_CONCEPT_ABSTRACT_H

#include "ttx/concept/callable.h"
#include "ttx/concept/documentation.h"
#include "ttx/concept/interface.h"

// Abstract is the common capability carried by every semantic identity. Its
// universal table follows the useful part of COM IUnknown: one stable identity
// answers the questions every consumer may ask. TTX leaves lifetime with the
// graph owner and returns borrowed category views instead of adjusted and
// reference counted interface pointers.
//
// Keeping this table small matters because every Dialect composes it into its
// real objects. Rich declaration policy stays with those owners, while tools
// can still describe and explore a partial graph without constructing a common
// declaration record.
struct ttx_abstract_operations {
  // Name and Documentation describe the identity even when later semantic
  // edges remain incomplete. Editor consumers therefore keep useful authored
  // context without treating a declaration projection as shared meaning.
  struct perimortem_bytes (*name)(const struct ttx_abstract* self);
  const struct ttx_documentation* (*documentation)(
      const struct ttx_abstract* self);

  // Resolution follows represented identity only. Concept lookup instead
  // lends one binary question to the current receiver, which lets concrete
  // syntax cross package, source, and type contexts without a global route
  // table or a flat access mode.
  const struct ttx_abstract* (*resolve)(const struct ttx_abstract* self);
  const struct ttx_abstract* (*resolve_concept)(
      const struct ttx_abstract* self,
      struct perimortem_bytes name);

  // Exploration reports the receiver's currently factual questions through a
  // synchronous Callable. The receiver retains neither the visitor nor an
  // ordered snapshot because later source observations may reveal more facts.
  void (*visit_concepts)(
      const struct ttx_abstract* self,
      struct ttx_named_abstract_callable* visitor);

  // Type is total so uncertainty remains a semantic answer rather than null.
  // Incomplete owners return Unknown and completed owners return their exact
  // Type fact.
  const struct ttx_abstract* (*type)(const struct ttx_abstract* self);

  // Category proof follows the witness table model used by trait and protocol
  // systems. The candidate keeps its exact identity while the returned view
  // supplies only the requested operations, avoiding native casts and a global
  // type registry.
  struct ttx_interface (*interface)(
      const struct ttx_abstract* self,
      const struct ttx_abstract* requirement);
};

// The immutable operation pointer comes first so a concrete owner can embed
// this handle directly in its real object. Its address remains local identity
// for the graph lifetime, and the graph owner retains every object borrowed
// through this surface.
struct ttx_abstract {
  const struct ttx_abstract_operations* operations;
};

PERIMORTEM_EXTERN_C struct perimortem_bytes ttx_abstract_name(
    const struct ttx_abstract* abstract);
PERIMORTEM_EXTERN_C const struct ttx_documentation* ttx_abstract_documentation(
    const struct ttx_abstract* abstract);
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
