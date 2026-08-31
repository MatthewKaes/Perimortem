// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include <stddef.h>

#include "perimortem/core/object.h"

#include "perimortem/system/terminal.h"

#include "ttx/concept/none.h"
#include "ttx/concept/unknown.h"
#include "ttx/model/callable.h"
#include "ttx/model/context.h"
#include "ttx/model/layouts/addressable.h"
#include "ttx/model/layouts/composite.h"
#include "ttx/model/layouts/fluid.h"
#include "ttx/model/layouts/named.h"
#include "ttx/model/layouts/ranged.h"
#include "ttx/model/layouts/termination.h"
#include "ttx/model/layouts/value.h"
#include "ttx/model/type.h"

#define TTX_CONTAINER_OF(pointer, type, member) \
  ((type*)((uint8_t*)(pointer) - offsetof(type, member)))

_Static_assert(sizeof(perimortem_bool) == 1, "boolean ABI width");
_Static_assert(sizeof(perimortem_count) == 8, "count ABI width");
_Static_assert(
    sizeof(struct perimortem_bytes) ==
        sizeof(const uint8_t*) + sizeof(perimortem_count),
    "byte view ABI shape");
_Static_assert(
    offsetof(struct perimortem_object_descriptor, size) == 0,
    "object descriptor starts with size");
_Static_assert(
    offsetof(struct perimortem_terminal_line, value) == 0,
    "terminal result starts with its value");
_Static_assert(
    offsetof(struct perimortem_terminal_line, present) ==
        sizeof(struct perimortem_bytes),
    "terminal result state follows its value");
_Static_assert(
    offsetof(struct ttx_abstract, operations) == 0,
    "Abstract starts with its operation table");
_Static_assert(
    offsetof(struct ttx_layout, operations) == 0,
    "Layout starts with its operation table");
_Static_assert(
    offsetof(struct ttx_pack, operations) == 0,
    "Pack starts with its operation table");
_Static_assert(
    offsetof(struct ttx_context, operations) == 0,
    "Context starts with its operation table");
_Static_assert(
    offsetof(struct ttx_type_operations, interface) == 0,
    "Type operations start with Interface proof");
_Static_assert(
    offsetof(struct ttx_addressable_operations, interface) == 0,
    "Addressable operations start with Interface proof");
_Static_assert(
    offsetof(struct ttx_callable_operations, interface) == 0,
    "Callable operations start with Interface proof");
_Static_assert(
    offsetof(struct ttx_named_layout_operations, interface) == 0,
    "Named Layout operations start with Interface proof");
_Static_assert(
    offsetof(struct ttx_abstract_callable, operations) == 0,
    "Abstract Callable starts with its operation table");
_Static_assert(
    offsetof(struct ttx_named_abstract_callable, operations) == 0,
    "Named Abstract Callable starts with its operation table");
_Static_assert(
    offsetof(struct ttx_bytes_callable, operations) == 0,
    "Bytes Callable starts with its operation table");

struct test_type {
  struct ttx_abstract abstract;
  struct ttx_type_operations type;
  struct ttx_value_layout layout;
  struct perimortem_bytes name;
  struct perimortem_bytes concept_name;
  const struct ttx_abstract* concept_target;
};

static struct perimortem_bytes type_name(const struct ttx_abstract* base) {
  return TTX_CONTAINER_OF(base, const struct test_type, abstract)->name;
}

static const struct ttx_documentation* documentation(
    const struct ttx_abstract* base) {
  (void)base;
  return ttx_documentation_empty();
}

static const struct ttx_abstract* identity(const struct ttx_abstract* base) {
  return base;
}

static const struct ttx_abstract* unknown_concept(
    const struct ttx_abstract* base,
    struct perimortem_bytes name) {
  const struct test_type* type =
      TTX_CONTAINER_OF(base, const struct test_type, abstract);
  if (type->concept_target != 0 &&
      perimortem_bytes_equal(type->concept_name, name)) {
    return type->concept_target;
  }

  return ttx_unknown();
}

static void no_concepts(
    const struct ttx_abstract* base,
    struct ttx_named_abstract_callable* visitor) {
  const struct test_type* type =
      TTX_CONTAINER_OF(base, const struct test_type, abstract);
  if (type->concept_target != 0) {
    ttx_named_abstract_callable_call(
        visitor, type->concept_name, type->concept_target);
  }
}

static ttx_interface_relation satisfied(
    const struct ttx_abstract* requirement,
    const struct ttx_abstract* candidate) {
  (void)requirement;
  (void)candidate;
  return TTX_INTERFACE_SATISFIED;
}

static const struct ttx_layout* type_layout(const struct ttx_abstract* base) {
  return &TTX_CONTAINER_OF(base, const struct test_type, abstract)
              ->layout.layout;
}

static struct ttx_interface type_interface(
    const struct ttx_abstract* base,
    const struct ttx_abstract* requirement) {
  const struct test_type* self =
      TTX_CONTAINER_OF(base, const struct test_type, abstract);
  if (requirement == ttx_type_requirement()) {
    return ttx_interface_satisfied(requirement, base, &self->type.interface);
  }

  if (requirement == ttx_abstract_requirement()) {
    return ttx_interface_satisfied(
        requirement, base, ttx_interface_marker_operations());
  }

  return ttx_interface_rejected(requirement, base);
}

static const struct ttx_abstract_operations abstract_operations = {
  .name = type_name,
  .documentation = documentation,
  .resolve = identity,
  .type = identity,
  .resolve_concept = unknown_concept,
  .visit_concepts = no_concepts,
  .interface = type_interface,
};

static void initialize_type(
    struct test_type* type,
    const uint8_t* name,
    perimortem_count size) {
  type->abstract.operations = &abstract_operations;
  type->type.interface.negotiate = satisfied;
  type->type.layout = type_layout;
  type->name.data = name;
  type->name.size = size;
  type->concept_name = (struct perimortem_bytes){0, 0};
  type->concept_target = 0;
  ttx_value_layout_initialize(&type->layout, &type->abstract);
}

struct entry_counter {
  struct ttx_abstract_callable callable;
  perimortem_count count;
};

struct named_capture {
  struct ttx_named_abstract_callable callable;
  struct perimortem_bytes first_name;
  const struct ttx_abstract* first_entry;
  perimortem_count count;
};

static void capture_named(
    struct ttx_named_abstract_callable* callable,
    struct perimortem_bytes name,
    const struct ttx_abstract* entry) {
  struct named_capture* capture =
      TTX_CONTAINER_OF(callable, struct named_capture, callable);
  if (capture->count == 0) {
    capture->first_name = name;
    capture->first_entry = entry;
  }

  ++capture->count;
}

static const struct ttx_named_abstract_callable_operations capture_operations =
    {
      .call = capture_named,
};

static void count_entry(
    struct ttx_abstract_callable* callable,
    const struct ttx_abstract* entry) {
  struct entry_counter* counter =
      TTX_CONTAINER_OF(callable, struct entry_counter, callable);
  (void)entry;
  ++counter->count;
}

static const struct ttx_abstract_callable_operations counter_operations = {
  .call = count_entry,
};

int main(void) {
  static const uint8_t left_name[] = "Left";
  static const uint8_t right_name[] = "Right";
  static const uint8_t first_name[] = "first";
  static const uint8_t second_name[] = "second";
  static const uint8_t peer_name[] = "peer";
  static const uint8_t fold_name[] = "fold";
  static const uint8_t unrelated_name[] = "unrelated";
  struct test_type left;
  struct test_type right;
  struct ttx_type_view left_view;
  struct ttx_constant_view constant;
  const struct ttx_abstract* source_entries[2];
  const struct ttx_abstract* target_entries[2];
  const struct ttx_abstract* repeated_entries[2];
  const struct ttx_abstract* composite_entries[2];
  struct perimortem_bytes source_names[2];
  struct perimortem_bytes target_names[2];
  struct ttx_fluid_layout source;
  struct ttx_fluid_layout target;
  struct ttx_fluid_layout empty;
  struct ttx_fluid_layout repeated;
  struct ttx_fluid_layout composite_target;
  struct ttx_ranged_layout ranged;
  struct ttx_named_layout named_source;
  struct ttx_named_layout named_target;
  struct ttx_layout_interface layout_interface;
  struct ttx_composite_layout composite;
  struct ttx_layout_addressable first_slot;
  struct ttx_layout_addressable second_slot;
  const struct ttx_abstract* slot_entries[2];
  struct ttx_addressable_layout slots;
  struct ttx_addressable_view slot;
  struct ttx_model_context context;
  struct ttx_model_pack context_packs[1];
  struct ttx_fluid_layout context_layouts[1];
  struct ttx_named_layout context_named_layouts[1];
  const struct ttx_abstract* context_entries[2];
  struct perimortem_bytes context_names[2];
  uint8_t context_name_bytes[16];
  const struct ttx_pack* snapshot;
  struct ttx_named_layout_view snapshot_named;
  struct named_capture capture = {
    .callable = {.operations = &capture_operations},
    .first_name = {0, 0},
    .first_entry = 0,
    .count = 0,
  };
  struct entry_counter counter = {
    .callable = {.operations = &counter_operations},
    .count = 0,
  };

  initialize_type(&left, left_name, sizeof(left_name) - 1);
  initialize_type(&right, right_name, sizeof(right_name) - 1);
  left.concept_name =
      (struct perimortem_bytes){peer_name, sizeof(peer_name) - 1};
  left.concept_target = &right.abstract;
  if (!ttx_type_prove(&left.abstract, &left_view) ||
      !ttx_type_layout_terminates(&left_view) ||
      !ttx_constant_prove(ttx_none(), &constant) ||
      ttx_constant_prove(ttx_unknown(), &constant)) {
    return 1;
  }

  ttx_abstract_visit_concepts(&left.abstract, &capture.callable);
  if (capture.count != 1 || capture.first_entry != &right.abstract ||
      !perimortem_bytes_equal(capture.first_name, left.concept_name) ||
      ttx_abstract_resolve_concept(
          &left.abstract,
          (struct perimortem_bytes){fold_name, sizeof(fold_name) - 1}) !=
          ttx_unknown()) {
    return 2;
  }

  right.concept_name =
      (struct perimortem_bytes){fold_name, sizeof(fold_name) - 1};
  right.concept_target = ttx_none();
  if (ttx_abstract_resolve_concept(
          &right.abstract,
          (struct perimortem_bytes){fold_name, sizeof(fold_name) - 1}) !=
          ttx_none() ||
      ttx_abstract_resolve_concept(
          ttx_none(),
          (struct perimortem_bytes){
            unrelated_name, sizeof(unrelated_name) - 1}) != ttx_unknown() ||
      !ttx_constant_prove(
          ttx_abstract_resolve_concept(
              &right.abstract,
              (struct perimortem_bytes){fold_name, sizeof(fold_name) - 1}),
          &constant)) {
    return 3;
  }

  capture.count = 0;
  capture.first_entry = 0;
  capture.first_name = (struct perimortem_bytes){0, 0};

  source_entries[0] = &left.abstract;
  source_entries[1] = &right.abstract;
  target_entries[0] = &right.abstract;
  target_entries[1] = &left.abstract;
  ttx_fluid_layout_initialize(&source, source_entries, 2);
  ttx_fluid_layout_initialize(&target, target_entries, 2);
  if (ttx_layout_fits(&source.layout, &target.layout)) {
    return 4;
  }

  source_names[0] =
      (struct perimortem_bytes){first_name, sizeof(first_name) - 1};
  source_names[1] =
      (struct perimortem_bytes){second_name, sizeof(second_name) - 1};
  target_names[0] = source_names[1];
  target_names[1] = source_names[0];
  ttx_named_layout_initialize(&named_source, &source.layout, source_names, 2);
  ttx_named_layout_initialize(&named_target, &target.layout, target_names, 2);
  layout_interface = ttx_layout_negotiate_interface(
      &source.layout, ttx_named_layout_requirement());
  if (ttx_layout_interface_accepts(&layout_interface)) {
    return 5;
  }

  layout_interface = ttx_layout_negotiate_interface(
      &named_source.layout, ttx_named_layout_requirement());
  if (!ttx_layout_interface_accepts(&layout_interface)) {
    return 5;
  }

  if (!ttx_layout_fits(&named_source.layout, &named_target.layout)) {
    return 5;
  }

  ttx_fluid_layout_initialize(&empty, 0, 0);
  ttx_ranged_layout_initialize(&ranged, &left.abstract, 2);
  repeated_entries[0] = &left.abstract;
  repeated_entries[1] = &left.abstract;
  ttx_fluid_layout_initialize(&repeated, repeated_entries, 2);
  if (!ttx_layout_fits(&empty.layout, &empty.layout) ||
      !ttx_layout_fits(&ranged.layout, &repeated.layout)) {
    return 6;
  }

  ttx_layout_addressable_initialize(
      &first_slot, source_names[0], &left.abstract);
  ttx_layout_addressable_initialize(
      &second_slot, source_names[1], &right.abstract);
  slot_entries[0] = &first_slot.abstract;
  slot_entries[1] = &second_slot.abstract;
  ttx_addressable_layout_initialize(&slots, slot_entries, source_names, 2);
  if (!ttx_addressable_prove(slot_entries[0], &slot) ||
      ttx_addressable_type(&slot) != &left.abstract ||
      !ttx_named_layout_prove(&slots.named.layout, &snapshot_named)) {
    return 7;
  }

  composite_entries[0] = &left.abstract;
  composite_entries[1] = &right.abstract;
  ttx_fluid_layout_initialize(&composite_target, composite_entries, 2);
  ttx_composite_layout_initialize(
      &composite, &left.layout.layout, &right.layout.layout);
  if (!ttx_layout_fits(&composite.layout, &composite_target.layout)) {
    return 8;
  }

  ttx_model_context_initialize(
      &context, (struct ttx_model_context_storage){
                  .packs = context_packs,
                  .layouts = context_layouts,
                  .named_layouts = context_named_layouts,
                  .entries = context_entries,
                  .names = context_names,
                  .name_bytes = context_name_bytes,
                  .pack_capacity = 1,
                  .entry_capacity = 2,
                  .name_capacity = 2,
                  .name_byte_capacity = sizeof(context_name_bytes),
                });
  snapshot = ttx_context_pack(&context.context, &named_source.layout);
  if (snapshot == 0) {
    return 9;
  }

  source_entries[0] = &right.abstract;
  source_names[0] = source_names[1];
  if (!ttx_layout_fits(ttx_pack_layout(snapshot), &named_target.layout) ||
      !ttx_named_layout_prove(ttx_pack_layout(snapshot), &snapshot_named)) {
    return 10;
  }

  ttx_named_layout_visit(&snapshot_named, &capture.callable);
  if (capture.count != 2 || capture.first_entry != &left.abstract ||
      !perimortem_bytes_equal(
          capture.first_name,
          (struct perimortem_bytes){first_name, sizeof(first_name) - 1})) {
    return 11;
  }

  ttx_layout_visit(&composite.layout, &counter.callable);
  return counter.count == 2 ? 0 : 12;
}
