// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#ifndef TTX_CONCEPT_LAYOUT_H
#define TTX_CONCEPT_LAYOUT_H

#include "ttx/concept/callable.h"

struct ttx_layout;

// Layout extensions use canonical requirement handles rather than optional
// fields on the base carrier. This follows the same witness principle as
// Abstract Interface proof while keeping Layout identity free. A requirement
// has no numeric kind or registry entry. Its stable address identifies the
// exact support contract within one runtime.
struct ttx_layout_interface_requirement {
  uint8_t reserved;
};

struct ttx_layout_interface_operations {
  const struct ttx_layout_interface_requirement* (*requirement)(void);
};

struct ttx_layout_interface {
  const struct ttx_layout* layout;
  const struct ttx_layout_interface_requirement* requirement;
  const struct ttx_layout_interface_operations* operations;
};

// Visit and fits remain the complete semantic Layout API. Interface is the
// proof mechanism for richer support views such as Named Layout and does not
// add their operations or ordering assumptions to every Layout.
struct ttx_layout_operations {
  void (*visit)(
      const struct ttx_layout* self,
      struct ttx_abstract_callable* visitor);
  perimortem_bool (
      *fits)(const struct ttx_layout* self, const struct ttx_layout* target);
  struct ttx_layout_interface (*interface)(
      const struct ttx_layout* self,
      const struct ttx_layout_interface_requirement* requirement);
};

struct ttx_layout {
  const struct ttx_layout_operations* operations;
};

PERIMORTEM_EXTERN_C void ttx_layout_visit(
    const struct ttx_layout* layout,
    struct ttx_abstract_callable* visitor);
PERIMORTEM_EXTERN_C perimortem_bool ttx_layout_fits(
    const struct ttx_layout* source,
    const struct ttx_layout* target);
PERIMORTEM_EXTERN_C struct ttx_layout_interface ttx_layout_negotiate_interface(
    const struct ttx_layout* layout,
    const struct ttx_layout_interface_requirement* requirement);
PERIMORTEM_EXTERN_C perimortem_bool
    ttx_layout_interface_accepts(const struct ttx_layout_interface* interface);
PERIMORTEM_EXTERN_C struct ttx_layout_interface ttx_layout_interface_rejected(
    const struct ttx_layout* layout,
    const struct ttx_layout_interface_requirement* requirement);

#endif
