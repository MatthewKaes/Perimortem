// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "ttx/model/layouts/termination.h"

#include <stddef.h>

#include "ttx/model/addressable.h"

#define TTX_CONTAINER_OF(pointer, type, member) \
  ((type*)((uint8_t*)(pointer)-offsetof(type, member)))

typedef struct termination_frame termination_frame;
struct termination_frame {
  const ttx_abstract* type;
  const termination_frame* parent;
};

typedef struct termination_visitor {
  ttx_abstract_callable callable;
  const termination_frame* frame;
  perimortem_count layout_count;
  perimortem_bool valid;
} termination_visitor;

typedef struct layout_counter {
  ttx_abstract_callable callable;
  perimortem_count count;
} layout_counter;

static void count_entry(
    ttx_abstract_callable* callable,
    const ttx_abstract* entry) {
  layout_counter* counter =
      TTX_CONTAINER_OF(callable, layout_counter, callable);
  (void)entry;
  ++counter->count;
}

static const ttx_abstract_callable_operations counter_operations = {
    .call = count_entry,
};

static perimortem_bool terminates(
    const ttx_type_view* type,
    const termination_frame* parent);

static perimortem_bool active(
    const termination_frame* frame,
    const ttx_abstract* type) {
  while (frame != 0) {
    if (frame->type == type) {
      return PERIMORTEM_TRUE;
    }
    frame = frame->parent;
  }
  return PERIMORTEM_FALSE;
}

static const ttx_abstract* entry_type(const ttx_abstract* entry) {
  ttx_type_view type;
  ttx_addressable_view addressable;
  const ttx_abstract* selected;

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
    ttx_abstract_callable* callable,
    const ttx_abstract* entry) {
  termination_visitor* visitor =
      TTX_CONTAINER_OF(callable, termination_visitor, callable);
  const ttx_abstract* selected;
  ttx_type_view type;

  if (!visitor->valid) {
    return;
  }
  selected = entry_type(entry);
  if (selected == 0) {
    visitor->valid = PERIMORTEM_FALSE;
    return;
  }
  if (selected == visitor->frame->type) {
    visitor->valid = visitor->layout_count == 1 ? PERIMORTEM_TRUE
                                                : PERIMORTEM_FALSE;
    return;
  }
  if (active(visitor->frame, selected) || !ttx_type_prove(selected, &type) ||
      !terminates(&type, visitor->frame)) {
    visitor->valid = PERIMORTEM_FALSE;
  }
}

static const ttx_abstract_callable_operations visitor_operations = {
    .call = visit_entry,
};

static perimortem_bool terminates(
    const ttx_type_view* type,
    const termination_frame* parent) {
  const ttx_layout* layout = ttx_type_layout(type);
  layout_counter counter = {
      .callable = {.operations = &counter_operations},
      .count = 0,
  };
  termination_frame frame = {
      .type = type->identity,
      .parent = parent,
  };
  termination_visitor visitor = {
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

perimortem_bool ttx_type_layout_terminates(const ttx_type_view* type) {
  return terminates(type, 0);
}
