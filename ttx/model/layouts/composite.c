// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "ttx/model/layouts/composite.h"

#include <stddef.h>

#include "ttx/model/layouts/common_internal.h"

#define TTX_CONTAINER_OF(pointer, type, member) \
  ((type*)((uint8_t*)(pointer)-offsetof(type, member)))

struct layout_count {
  struct ttx_abstract_callable callable;
  perimortem_count count;
};

static void count_entry(
    struct ttx_abstract_callable* callable,
    const struct ttx_abstract* entry) {
  struct layout_count* count = TTX_CONTAINER_OF(callable, struct layout_count, callable);
  (void)entry;
  ++count->count;
}

static const struct ttx_abstract_callable_operations count_operations = {
    .call = count_entry,
};

static perimortem_count count(const struct ttx_layout* layout) {
  struct layout_count result = {
      .callable = {.operations = &count_operations},
      .count = 0,
  };
  ttx_layout_visit(layout, &result.callable);
  return result.count;
}

struct segment_layout {
  struct ttx_layout layout;
  const struct ttx_layout* source;
  perimortem_count offset;
  perimortem_count count;
};

struct segment_visitor {
  struct ttx_abstract_callable callable;
  struct ttx_abstract_callable* target;
  perimortem_count offset;
  perimortem_count count;
  perimortem_count current;
};

static void visit_segment_entry(
    struct ttx_abstract_callable* callable,
    const struct ttx_abstract* entry) {
  struct segment_visitor* visitor =
      TTX_CONTAINER_OF(callable, struct segment_visitor, callable);
  if (visitor->current >= visitor->offset &&
      visitor->current - visitor->offset < visitor->count) {
    ttx_abstract_callable_call(visitor->target, entry);
  }
  ++visitor->current;
}

static const struct ttx_abstract_callable_operations segment_visitor_operations = {
    .call = visit_segment_entry,
};

static void visit_segment(
    const struct ttx_layout* base,
    struct ttx_abstract_callable* visitor) {
  const struct segment_layout* self =
      TTX_CONTAINER_OF(base, const struct segment_layout, layout);
  struct segment_visitor selected = {
      .callable = {.operations = &segment_visitor_operations},
      .target = visitor,
      .offset = self->offset,
      .count = self->count,
      .current = 0,
  };
  ttx_layout_visit(self->source, &selected.callable);
}

static perimortem_bool fit_segment(
    const struct ttx_layout* source,
    const struct ttx_layout* target) {
  return ttx_layout_entries_fit(source, target);
}

static const struct ttx_layout_operations segment_operations = {
    .visit = visit_segment,
    .fits = fit_segment,
};

static struct segment_layout segment(
    const struct ttx_layout* source,
    perimortem_count offset,
    perimortem_count length) {
  struct segment_layout result = {
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
    const struct ttx_layout* base,
    struct ttx_abstract_callable* visitor) {
  const struct ttx_composite_layout* self =
      TTX_CONTAINER_OF(base, const struct ttx_composite_layout, layout);
  ttx_layout_visit(self->first, visitor);
  ttx_layout_visit(self->second, visitor);
}

static perimortem_bool fits(
    const struct ttx_layout* base,
    const struct ttx_layout* target) {
  const struct ttx_composite_layout* self =
      TTX_CONTAINER_OF(base, const struct ttx_composite_layout, layout);
  const perimortem_count first_count = count(self->first);
  const perimortem_count second_count = count(self->second);
  struct segment_layout first_target;
  struct segment_layout second_target;
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

static const struct ttx_layout_operations operations = {
    .visit = visit,
    .fits = fits,
};

void ttx_composite_layout_initialize(
    struct ttx_composite_layout* layout,
    const struct ttx_layout* first,
    const struct ttx_layout* second) {
  layout->layout.operations = &operations;
  layout->layout.named = 0;
  layout->first = first;
  layout->second = second;
}
