// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "ttx/model/layouts/termination.h"

#include <stddef.h>

#include "ttx/model/addressable.h"

#define TTX_CONTAINER_OF(pointer, type, member) \
  ((type*)((uint8_t*)(pointer) - offsetof(type, member)))

struct termination_frame;
struct termination_frame {
  const struct ttx_abstract* type;
  const struct termination_frame* parent;
};

struct termination_visitor {
  struct ttx_abstract_callable callable;
  const struct termination_frame* frame;
  perimortem_count layout_count;
  perimortem_bool valid;
};

struct layout_counter {
  struct ttx_abstract_callable callable;
  perimortem_count count;
};

static void count_entry(
    struct ttx_abstract_callable* callable,
    const struct ttx_abstract* entry) {
  struct layout_counter* counter =
      TTX_CONTAINER_OF(callable, struct layout_counter, callable);
  (void)entry;
  ++counter->count;
}

static const struct ttx_abstract_callable_operations counter_operations = {
  .call = count_entry,
};

static perimortem_bool terminates(
    const struct ttx_type_view* type,
    const struct termination_frame* parent);

static perimortem_bool active(
    const struct termination_frame* frame,
    const struct ttx_abstract* type) {
  while (frame != 0) {
    if (frame->type == type) {
      return PERIMORTEM_TRUE;
    }

    frame = frame->parent;
  }

  return PERIMORTEM_FALSE;
}

static const struct ttx_abstract* entry_type(const struct ttx_abstract* entry) {
  struct ttx_type_view type;
  struct ttx_addressable_view addressable;
  const struct ttx_abstract* selected;

  if (ttx_type_prove(entry, &type)) {
    return entry;
  }

  if (ttx_addressable_prove(entry, &addressable)) {
    return ttx_addressable_type(&addressable);
  }

  selected = ttx_abstract_type(entry);
  return ttx_type_prove(selected, &type) ? selected : 0;
}

static void visit_entry(
    struct ttx_abstract_callable* callable,
    const struct ttx_abstract* entry) {
  struct termination_visitor* visitor =
      TTX_CONTAINER_OF(callable, struct termination_visitor, callable);
  const struct ttx_abstract* selected;
  struct ttx_type_view type;

  if (!visitor->valid) {
    return;
  }

  selected = entry_type(entry);
  if (selected == 0) {
    visitor->valid = PERIMORTEM_FALSE;
    return;
  }

  if (selected == visitor->frame->type) {
    visitor->valid =
        visitor->layout_count == 1 ? PERIMORTEM_TRUE : PERIMORTEM_FALSE;
    return;
  }

  if (active(visitor->frame, selected) || !ttx_type_prove(selected, &type) ||
      !terminates(&type, visitor->frame)) {
    visitor->valid = PERIMORTEM_FALSE;
  }
}

static const struct ttx_abstract_callable_operations visitor_operations = {
  .call = visit_entry,
};

static perimortem_bool terminates(
    const struct ttx_type_view* type,
    const struct termination_frame* parent) {
  const struct ttx_layout* layout = ttx_type_layout(type);
  struct layout_counter counter = {
    .callable = {.operations = &counter_operations},
    .count = 0,
  };
  struct termination_frame frame = {
    .type = type->identity,
    .parent = parent,
  };
  struct termination_visitor visitor = {
    .callable = {.operations = &visitor_operations},
    .frame = &frame,
    .layout_count = 0,
    .valid = PERIMORTEM_FALSE,
  };

  ttx_layout_visit(layout, &counter.callable);
  visitor.layout_count = counter.count;
  visitor.valid = counter.count == 0 ? PERIMORTEM_FALSE : PERIMORTEM_TRUE;
  if (!visitor.valid) {
    return PERIMORTEM_FALSE;
  }

  ttx_layout_visit(layout, &visitor.callable);
  return visitor.valid;
}

perimortem_bool ttx_type_layout_terminates(const struct ttx_type_view* type) {
  return terminates(type, 0);
}
