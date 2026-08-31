// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "ttx/concept/layout.h"

void ttx_layout_visit(
    const struct ttx_layout* layout,
    struct ttx_abstract_callable* visitor) {
  layout->operations->visit(layout, visitor);
}

perimortem_bool ttx_layout_fits(
    const struct ttx_layout* source,
    const struct ttx_layout* target) {
  return source->operations->fits(source, target);
}

struct ttx_layout_interface ttx_layout_negotiate_interface(
    const struct ttx_layout* layout,
    const struct ttx_layout_interface_requirement* requirement) {
  return layout->operations->interface(layout, requirement);
}

perimortem_bool ttx_layout_interface_accepts(
    const struct ttx_layout_interface* interface) {
  return interface != 0 && interface->layout != 0 &&
         interface->requirement != 0 && interface->operations != 0 &&
         interface->operations->requirement() == interface->requirement;
}

struct ttx_layout_interface ttx_layout_interface_rejected(
    const struct ttx_layout* layout,
    const struct ttx_layout_interface_requirement* requirement) {
  const struct ttx_layout_interface rejected = {
    .layout = layout,
    .requirement = requirement,
    .operations = 0,
  };
  return rejected;
}
