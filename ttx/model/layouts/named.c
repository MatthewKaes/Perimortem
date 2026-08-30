// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "ttx/model/layouts/named.h"

#include <stddef.h>

#include "ttx/model/layouts/common_internal.h"

#define TTX_CONTAINER_OF(pointer, type, member) \
  ((type*)((uint8_t*)(pointer)-offsetof(type, member)))

struct entry_selection {
  struct ttx_abstract_callable callable;
  perimortem_count requested;
  perimortem_count current;
  const struct ttx_abstract* selected;
};

static void select_entry(
    struct ttx_abstract_callable* callable,
    const struct ttx_abstract* entry) {
  struct entry_selection* selection =
      TTX_CONTAINER_OF(callable, struct entry_selection, callable);
  if (selection->current == selection->requested) {
    selection->selected = entry;
  }
  ++selection->current;
}

static const struct ttx_abstract_callable_operations selection_operations = {
    .call = select_entry,
};

static const struct ttx_abstract* entry_at(
    const struct ttx_layout* layout,
    perimortem_count index) {
  struct entry_selection selection = {
      .callable = {.operations = &selection_operations},
      .requested = index,
      .current = 0,
      .selected = 0,
  };
  ttx_layout_visit(layout, &selection.callable);
  return selection.selected;
}

static void visit(
    const struct ttx_layout* base,
    struct ttx_abstract_callable* visitor) {
  const struct ttx_named_layout* self =
      TTX_CONTAINER_OF(base, const struct ttx_named_layout, layout);
  ttx_layout_visit(self->source, visitor);
}

struct duplicate_name {
  struct ttx_named_abstract_callable callable;
  struct perimortem_bytes requested;
  perimortem_count matches;
};

static void count_name(
    struct ttx_named_abstract_callable* callable,
    struct perimortem_bytes name,
    const struct ttx_abstract* entry) {
  struct duplicate_name* duplicate =
      TTX_CONTAINER_OF(callable, struct duplicate_name, callable);
  (void)entry;
  if (perimortem_bytes_equal(duplicate->requested, name)) {
    ++duplicate->matches;
  }
}

static const struct ttx_named_abstract_callable_operations duplicate_operations = {
    .call = count_name,
};

struct validate_names {
  struct ttx_named_abstract_callable callable;
  const struct ttx_named_layout_view* view;
  perimortem_count count;
  perimortem_bool valid;
};

static void validate_name(
    struct ttx_named_abstract_callable* callable,
    struct perimortem_bytes name,
    const struct ttx_abstract* entry) {
  struct validate_names* validation =
      TTX_CONTAINER_OF(callable, struct validate_names, callable);
  struct duplicate_name duplicate = {
      .callable = {.operations = &duplicate_operations},
      .requested = name,
      .matches = 0,
  };
  (void)entry;
  ++validation->count;
  if (name.size == 0) {
    validation->valid = PERIMORTEM_FALSE;
    return;
  }
  ttx_named_layout_visit(validation->view, &duplicate.callable);
  if (duplicate.matches != 1) {
    validation->valid = PERIMORTEM_FALSE;
  }
}

static const struct ttx_named_abstract_callable_operations validation_operations = {
    .call = validate_name,
};

static perimortem_bool has_valid_names(
    const struct ttx_named_layout_view* view,
    perimortem_count* count) {
  struct validate_names validation = {
      .callable = {.operations = &validation_operations},
      .view = view,
      .count = 0,
      .valid = PERIMORTEM_TRUE,
  };
  ttx_named_layout_visit(view, &validation.callable);
  *count = validation.count;
  return validation.valid;
}

struct matching_entry {
  struct ttx_named_abstract_callable callable;
  struct perimortem_bytes name;
  const struct ttx_abstract* source;
  perimortem_count matches;
};

static void match_entry(
    struct ttx_named_abstract_callable* callable,
    struct perimortem_bytes name,
    const struct ttx_abstract* entry) {
  struct matching_entry* matching =
      TTX_CONTAINER_OF(callable, struct matching_entry, callable);
  if (perimortem_bytes_equal(matching->name, name) &&
      ttx_layout_entry_fits(matching->source, entry)) {
    ++matching->matches;
  }
}

static const struct ttx_named_abstract_callable_operations matching_operations = {
    .call = match_entry,
};

static perimortem_bool fits(
    const struct ttx_layout* base,
    const struct ttx_layout* target) {
  const struct ttx_named_layout* source =
      TTX_CONTAINER_OF(base, const struct ttx_named_layout, layout);
  struct ttx_named_layout_view source_view;
  struct ttx_named_layout_view target_view;
  perimortem_count source_index;
  perimortem_count source_count;
  perimortem_count target_count;

  if (!ttx_named_layout_prove(base, &source_view) ||
      !ttx_named_layout_prove(target, &target_view) ||
      !has_valid_names(&source_view, &source_count) ||
      !has_valid_names(&target_view, &target_count) ||
      source_count != target_count || source_count != source->count) {
    return PERIMORTEM_FALSE;
  }
  for (source_index = 0; source_index < source->count; ++source_index) {
    struct matching_entry matching = {
        .callable = {.operations = &matching_operations},
        .name = source->names[source_index],
        .source = entry_at(source->source, source_index),
        .matches = 0,
    };
    ttx_named_layout_visit(&target_view, &matching.callable);
    if (matching.source == 0 || matching.matches != 1) {
      return PERIMORTEM_FALSE;
    }
  }
  return PERIMORTEM_TRUE;
}

static void visit_named(
    const struct ttx_layout* base,
    struct ttx_named_abstract_callable* visitor) {
  const struct ttx_named_layout* layout =
      TTX_CONTAINER_OF(base, const struct ttx_named_layout, layout);
  perimortem_count index;
  for (index = 0; index < layout->count; ++index) {
    const struct ttx_abstract* entry = entry_at(layout->source, index);
    if (entry != 0) {
      ttx_named_abstract_callable_call(
          visitor, layout->names[index], entry);
    }
  }
}

static const struct ttx_named_layout_operations named_operations = {
    .visit = visit_named,
};

static const struct ttx_layout_operations operations = {
    .visit = visit,
    .fits = fits,
};

void ttx_named_layout_initialize(
    struct ttx_named_layout* layout,
    const struct ttx_layout* source,
    const struct perimortem_bytes* names,
    perimortem_count count) {
  layout->layout.operations = &operations;
  layout->layout.named = &named_operations;
  layout->source = source;
  layout->names = names;
  layout->count = count;
}

perimortem_bool ttx_named_layout_prove(
    const struct ttx_layout* layout,
    struct ttx_named_layout_view* named_view) {
  if (layout->named == 0) {
    return PERIMORTEM_FALSE;
  }
  named_view->layout = layout;
  named_view->operations = layout->named;
  return PERIMORTEM_TRUE;
}

void ttx_named_layout_visit(
    const struct ttx_named_layout_view* layout,
    struct ttx_named_abstract_callable* visitor) {
  layout->operations->visit(layout->layout, visitor);
}
