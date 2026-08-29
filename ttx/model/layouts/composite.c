// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "ttx/model/layouts/composite.h"

#include <stddef.h>

#include "ttx/model/layouts/common_internal.h"

#define TTX_CONTAINER_OF(pointer, type, member) \
  ((type*)((uint8_t*)(pointer)-offsetof(type, member)))

typedef struct layout_count {
  ttx_abstract_callable callable;
  perimortem_count count;
} layout_count;

static void count_entry(
    ttx_abstract_callable* callable,
    const ttx_abstract* entry) {
  layout_count* count = TTX_CONTAINER_OF(callable, layout_count, callable);
  (void)entry;
  ++count->count;
}

static const ttx_abstract_callable_operations count_operations = {
    .call = count_entry,
};

static perimortem_count count(const ttx_layout* layout) {
  layout_count result = {
      .callable = {.operations = &count_operations},
      .count = 0,
  };
  ttx_layout_visit(layout, &result.callable);
  return result.count;
}

typedef struct segment_layout {
  ttx_layout layout;
  const ttx_layout* source;
  perimortem_count offset;
  perimortem_count count;
} segment_layout;

typedef struct segment_visitor {
  ttx_abstract_callable callable;
  ttx_abstract_callable* target;
  perimortem_count offset;
  perimortem_count count;
  perimortem_count current;
} segment_visitor;

static void visit_segment_entry(
    ttx_abstract_callable* callable,
    const ttx_abstract* entry) {
  segment_visitor* visitor =
      TTX_CONTAINER_OF(callable, segment_visitor, callable);
  if (visitor->current >= visitor->offset &&
      visitor->current - visitor->offset < visitor->count) {
    ttx_abstract_callable_call(visitor->target, entry);
  }
  ++visitor->current;
}

static const ttx_abstract_callable_operations segment_visitor_operations = {
    .call = visit_segment_entry,
};

static void visit_segment(
    const ttx_layout* base,
    ttx_abstract_callable* visitor) {
  const segment_layout* self =
      TTX_CONTAINER_OF(base, const segment_layout, layout);
  segment_visitor selected = {
      .callable = {.operations = &segment_visitor_operations},
      .target = visitor,
      .offset = self->offset,
      .count = self->count,
      .current = 0,
  };
  ttx_layout_visit(self->source, &selected.callable);
}

static perimortem_bool fit_segment(
    const ttx_layout* source,
    const ttx_layout* target) {
  return ttx_layout_entries_fit(source, target);
}

static const ttx_layout_operations segment_operations = {
    .visit = visit_segment,
    .fits = fit_segment,
};

static segment_layout segment(
    const ttx_layout* source,
    perimortem_count offset,
    perimortem_count length) {
  segment_layout result = {
      .layout = {
          .operations = &segment_operations,
          .named = 0,
      },
      .source = source,
      .offset = offset,
      .count = length,
  };
  return result;
}

static void visit(
    const ttx_layout* base,
    ttx_abstract_callable* visitor) {
  const ttx_composite_layout* self =
      TTX_CONTAINER_OF(base, const ttx_composite_layout, layout);
  ttx_layout_visit(self->first, visitor);
  ttx_layout_visit(self->second, visitor);
}

static perimortem_bool fits(
    const ttx_layout* base,
    const ttx_layout* target) {
  const ttx_composite_layout* self =
      TTX_CONTAINER_OF(base, const ttx_composite_layout, layout);
  const perimortem_count first_count = count(self->first);
  const perimortem_count second_count = count(self->second);
  segment_layout first_target;
  segment_layout second_target;
  if (count(target) != first_count + second_count) {
    return PERIMORTEM_FALSE;
  }
  first_target = segment(target, 0, first_count);
  second_target = segment(target, first_count, second_count);
  return ttx_layout_fits(self->first, &first_target.layout) &&
                 ttx_layout_fits(self->second, &second_target.layout)
             ? PERIMORTEM_TRUE
             : PERIMORTEM_FALSE;
}

static const ttx_layout_operations operations = {
    .visit = visit,
    .fits = fits,
};

void ttx_composite_layout_initialize(
    ttx_composite_layout* layout,
    const ttx_layout* first,
    const ttx_layout* second) {
  layout->layout.operations = &operations;
  layout->layout.named = 0;
  layout->first = first;
  layout->second = second;
}
