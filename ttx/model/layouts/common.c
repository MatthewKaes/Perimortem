// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "ttx/model/layouts/common_internal.h"

#include <stddef.h>

#include "ttx/model/addressable.h"
#include "ttx/model/type.h"

#define TTX_CONTAINER_OF(pointer, type, member) \
  ((type*)((uint8_t*)(pointer)-offsetof(type, member)))

struct count_entries {
  struct ttx_abstract_callable callable;
  perimortem_count count;
};

static void count_entry(
    struct ttx_abstract_callable* callable,
    const struct ttx_abstract* abstract) {
  struct count_entries* counter =
      TTX_CONTAINER_OF(callable, struct count_entries, callable);
  (void)abstract;
  ++counter->count;
}

static const struct ttx_abstract_callable_operations count_operations = {
    .call = count_entry,
};

struct select_entry {
  struct ttx_abstract_callable callable;
  perimortem_count requested;
  perimortem_count current;
  const struct ttx_abstract* selected;
};

static void select_current(
    struct ttx_abstract_callable* callable,
    const struct ttx_abstract* abstract) {
  struct select_entry* selection =
      TTX_CONTAINER_OF(callable, struct select_entry, callable);
  if (selection->current == selection->requested) {
    selection->selected = abstract;
  }
  ++selection->current;
}

static const struct ttx_abstract_callable_operations select_operations = {
    .call = select_current,
};

static const struct ttx_abstract* fitting_identity(const struct ttx_abstract* abstract) {
  struct ttx_type_view type;
  struct ttx_addressable_view addressable;
  const struct ttx_abstract* represented;

  if (ttx_type_prove(abstract, &type)) {
    return abstract;
  }
  if (ttx_addressable_prove(abstract, &addressable)) {
    return ttx_addressable_type(&addressable);
  }
  represented = ttx_abstract_resolve(abstract);
  if (ttx_addressable_prove(represented, &addressable)) {
    return ttx_addressable_type(&addressable);
  }
  return represented;
}

perimortem_bool ttx_layout_entry_fits(
    const struct ttx_abstract* source,
    const struct ttx_abstract* target) {
  return fitting_identity(source) == fitting_identity(target)
             ? PERIMORTEM_TRUE
             : PERIMORTEM_FALSE;
}

static perimortem_count entry_count(const struct ttx_layout* layout) {
  struct count_entries counter = {
      .callable = {.operations = &count_operations},
      .count = 0,
  };
  ttx_layout_visit(layout, &counter.callable);
  return counter.count;
}

static const struct ttx_abstract* entry_at(
    const struct ttx_layout* layout,
    perimortem_count index) {
  struct select_entry selection = {
      .callable = {.operations = &select_operations},
      .requested = index,
      .current = 0,
      .selected = 0,
  };
  ttx_layout_visit(layout, &selection.callable);
  return selection.selected;
}

perimortem_bool ttx_layout_entries_fit(
    const struct ttx_layout* source,
    const struct ttx_layout* target) {
  perimortem_count source_count = entry_count(source);
  perimortem_count target_count = entry_count(target);
  perimortem_count index;

  if (source_count != target_count) {
    return PERIMORTEM_FALSE;
  }
  for (index = 0; index < source_count; ++index) {
    const struct ttx_abstract* source_entry = entry_at(source, index);
    const struct ttx_abstract* target_entry = entry_at(target, index);
    if (source_entry == 0 || target_entry == 0 ||
        !ttx_layout_entry_fits(source_entry, target_entry)) {
      return PERIMORTEM_FALSE;
    }
  }
  return PERIMORTEM_TRUE;
}
