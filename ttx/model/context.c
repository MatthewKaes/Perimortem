// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "ttx/model/context.h"

#include <stddef.h>

#include "ttx/model/layouts/named.h"
#include "ttx/model/pack_internal.h"

#define TTX_CONTAINER_OF(pointer, type, member) \
  ((type*)((uint8_t*)(pointer)-offsetof(type, member)))

typedef struct copy_entries {
  ttx_abstract_callable callable;
  ttx_model_context* context;
  perimortem_bool valid;
} copy_entries;

typedef struct copy_named_entries {
  ttx_named_abstract_callable callable;
  ttx_model_context* context;
  perimortem_bool valid;
} copy_named_entries;

static void copy_entry(
    ttx_abstract_callable* callable,
    const ttx_abstract* entry) {
  copy_entries* copy =
      TTX_CONTAINER_OF(callable, copy_entries, callable);
  if (copy->context->entry_count ==
      copy->context->storage.entry_capacity) {
    copy->valid = PERIMORTEM_FALSE;
    return;
  }
  copy->context->storage.entries[copy->context->entry_count++] = entry;
}

static const ttx_abstract_callable_operations copy_operations = {
    .call = copy_entry,
};

static void copy_named_entry(
    ttx_named_abstract_callable* callable,
    perimortem_bytes name,
    const ttx_abstract* entry) {
  copy_named_entries* copy =
      TTX_CONTAINER_OF(callable, copy_named_entries, callable);
  ttx_model_context* context = copy->context;
  perimortem_count index;
  perimortem_bytes retained;
  if (context->entry_count == context->storage.entry_capacity ||
      context->name_count == context->storage.name_capacity ||
      name.size > context->storage.name_byte_capacity -
                      context->name_byte_count) {
    copy->valid = PERIMORTEM_FALSE;
    return;
  }
  retained.data = context->storage.name_bytes + context->name_byte_count;
  retained.size = name.size;
  for (index = 0; index < name.size; ++index) {
    context->storage.name_bytes[context->name_byte_count + index] =
        name.data[index];
  }
  context->name_byte_count += name.size;
  context->storage.entries[context->entry_count++] = entry;
  context->storage.names[context->name_count++] = retained;
}

static const ttx_named_abstract_callable_operations named_copy_operations = {
    .call = copy_named_entry,
};

static const ttx_pack* pack(
    ttx_context* base,
    const ttx_layout* layout) {
  ttx_model_context* self =
      TTX_CONTAINER_OF(base, ttx_model_context, context);
  ttx_named_layout_view named;
  perimortem_count pack_index;
  perimortem_count entry_start;
  perimortem_count name_start;
  perimortem_count byte_start;
  ttx_model_pack* selected;

  copy_entries entry_copy = {
      .callable = {.operations = &copy_operations},
      .context = self,
      .valid = PERIMORTEM_TRUE,
  };
  copy_named_entries named_copy = {
      .callable = {.operations = &named_copy_operations},
      .context = self,
      .valid = PERIMORTEM_TRUE,
  };

  if (self->pack_count == self->storage.pack_capacity) {
    return 0;
  }
  pack_index = self->pack_count;
  entry_start = self->entry_count;
  name_start = self->name_count;
  byte_start = self->name_byte_count;
  if (ttx_named_layout_prove(layout, &named)) {
    ttx_named_layout_visit(&named, &named_copy.callable);
    if (!named_copy.valid) {
      self->entry_count = entry_start;
      self->name_count = name_start;
      self->name_byte_count = byte_start;
      return 0;
    }
  } else {
    ttx_layout_visit(layout, &entry_copy.callable);
    if (!entry_copy.valid) {
      self->entry_count = entry_start;
      return 0;
    }
  }

  ttx_fluid_layout_initialize(
      &self->storage.layouts[pack_index],
      self->storage.entries + entry_start,
      self->entry_count - entry_start);
  selected = &self->storage.packs[pack_index];
  if (ttx_named_layout_prove(layout, &named)) {
    ttx_named_layout_initialize(
        &self->storage.named_layouts[pack_index],
        &self->storage.layouts[pack_index].layout,
        self->storage.names + name_start,
        self->name_count - name_start);
    ttx_model_pack_initialize(
        selected, &self->storage.named_layouts[pack_index].layout);
  } else {
    ttx_model_pack_initialize(
        selected, &self->storage.layouts[pack_index].layout);
  }
  ++self->pack_count;
  return &selected->pack;
}

static const ttx_context_operations operations = {
    .pack = pack,
};

void ttx_model_context_initialize(
    ttx_model_context* context,
    ttx_model_context_storage storage) {
  context->context.operations = &operations;
  context->storage = storage;
  context->pack_count = 0;
  context->entry_count = 0;
  context->name_count = 0;
  context->name_byte_count = 0;
}
