// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "ttx/concept/interface.h"

static ttx_interface_relation satisfied(
    const struct ttx_abstract* requirement,
    const struct ttx_abstract* candidate) {
  (void)requirement;
  (void)candidate;
  return TTX_INTERFACE_SATISFIED;
}

static const struct ttx_interface_operations marker_operations = {
  .negotiate = satisfied,
};

ttx_interface_relation ttx_interface_negotiate(
    const struct ttx_interface* interface) {
  if (interface->operations == 0) {
    return TTX_INTERFACE_REJECTED;
  }

  return interface->operations->negotiate(
      interface->requirement, interface->candidate);
}

perimortem_bool ttx_interface_accepts(const struct ttx_interface* interface) {
  return ttx_interface_negotiate(interface) != TTX_INTERFACE_REJECTED
             ? PERIMORTEM_TRUE
             : PERIMORTEM_FALSE;
}

struct ttx_interface ttx_interface_rejected(
    const struct ttx_abstract* requirement,
    const struct ttx_abstract* candidate) {
  struct ttx_interface interface = {
    .requirement = requirement,
    .candidate = candidate,
    .operations = 0,
  };
  return interface;
}

struct ttx_interface ttx_interface_satisfied(
    const struct ttx_abstract* requirement,
    const struct ttx_abstract* candidate,
    const struct ttx_interface_operations* operations) {
  struct ttx_interface interface = {
    .requirement = requirement,
    .candidate = candidate,
    .operations = operations,
  };
  return interface;
}

const struct ttx_interface_operations* ttx_interface_marker_operations(void) {
  return &marker_operations;
}
