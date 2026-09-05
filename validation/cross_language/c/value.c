// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "cross_language/c/value.h"

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#define ABI_HEADER(type)            \
  {                                 \
    .size = (uint32_t)sizeof(type), \
    .abi_major = TTX_ABI_MAJOR,     \
    .abi_minor = TTX_ABI_MINOR,     \
  }

struct value_state {
  ttx_abstract_capability capability;
  ttx_borrowed_bytes name;
  ttx_borrowed_bytes text;
  ttx_abstract value_requirement;
  ttx_abstract view_bytes_requirement;
  ttx_abstract to_string_operation;
};

struct value_interface_state {
  ttx_interface_capability capability;
  ttx_interface_ops operations;
  ttx_abstract requirement;
  ttx_abstract candidate;
  ttx_interface_relation relation;
};

struct value_snapshot {
  ttx_layout_snapshot_ops operations;
  struct value_state* value;
};

static const ttx_abstract_ops value_operations;
static const ttx_layout_ops layout_operations;
static const ttx_enumerable_ops enumerable_operations;
static const ttx_layout_snapshot_ops snapshot_operations;
static const ttx_bytes_ops bytes_operations;

static struct value_state* value_state(ttx_abstract value) {
  return (struct value_state*)value;
}

static ttx_abstract value_handle(struct value_state* value) {
  return &value->capability;
}

static ttx_layout value_layout(struct value_state* value) {
  const ttx_layout result = {
    .operations = &layout_operations,
    .self = (ttx_layout_self*)value,
  };
  return result;
}

static ttx_enumerable value_enumerable(struct value_state* value) {
  const ttx_enumerable result = {
    .operations = &enumerable_operations,
    .self = (ttx_enumerable_self*)value,
  };
  return result;
}

static ttx_abstract register_value(
    ttx_borrowed_bytes name,
    ttx_borrowed_bytes text,
    ttx_abstract value_requirement,
    ttx_abstract view_bytes_requirement,
    ttx_abstract to_string_operation) {
  struct value_state* value = malloc(sizeof(struct value_state));
  if (value == 0) {
    return ttx_unknown();
  }
  value->capability.operations = &value_operations;
  value->name = name;
  value->text = text;
  value->value_requirement = value_requirement;
  value->view_bytes_requirement = view_bytes_requirement;
  value->to_string_operation = to_string_operation;
  return value_handle(value);
}

static ttx_borrowed_bytes TTX_CALL value_name(ttx_abstract self) {
  struct value_state* value = value_state(self);
  return value == 0 ? (ttx_borrowed_bytes){0} : value->name;
}

static ttx_documentation TTX_CALL value_documentation(ttx_abstract self) {
  const ttx_abstract none = ttx_none();
  (void)self;
  return none->operations->documentation(none);
}

static void TTX_CALL
    value_resolve(ttx_abstract self, ttx_abstract_sink result) {
  result.operations->answer(result, self);
}

static void TTX_CALL value_resolve_concept(
    ttx_abstract self,
    ttx_borrowed_bytes route,
    ttx_abstract_sink result) {
  (void)self;
  (void)route;
  result.operations->answer(result, ttx_unknown());
}

static void TTX_CALL
    value_visit_concepts(ttx_abstract self, ttx_concept_sink result) {
  (void)self;
  result.operations->completed(result);
}

static struct value_interface_state* interface_state(ttx_interface self) {
  return (struct value_interface_state*)self;
}

static ttx_abstract TTX_CALL interface_requirement(ttx_interface self) {
  return interface_state(self)->requirement;
}

static ttx_abstract TTX_CALL interface_candidate(ttx_interface self) {
  return interface_state(self)->candidate;
}

static ttx_interface_relation TTX_CALL interface_negotiate(ttx_interface self) {
  return interface_state(self)->relation;
}

static void TTX_CALL interface_invoke(
    ttx_interface self,
    ttx_abstract operation,
    ttx_pack input,
    ttx_context context,
    ttx_pack_result result) {
  struct value_interface_state* interface = interface_state(self);
  struct value_state* value =
      interface == 0 ? 0 : value_state(interface->candidate);
  if (interface == 0 || value == 0 ||
      interface->relation != TTX_INTERFACE_SATISFIED ||
      !ttx_abstract_same(interface->requirement, value->value_requirement) ||
      !ttx_abstract_same(operation, value->to_string_operation) ||
      ttx_test_pack_cardinality(input) != 0) {
    result.operations->none(result);
    return;
  }
  context.operations->pack(context, value_layout(value), result);
}

static void TTX_CALL value_interface(
    ttx_abstract self,
    ttx_abstract requirement,
    ttx_interface_sink result) {
  struct value_state* value = value_state(self);
  if (value == 0) {
    return;
  }
  struct value_interface_state state = {
    .capability = {0},
    .operations =
        {
          .header = ABI_HEADER(ttx_interface_ops),
          .requirement = interface_requirement,
          .candidate = interface_candidate,
          .negotiate = interface_negotiate,
          .invoke = interface_invoke,
        },
    .requirement = requirement,
    .candidate = self,
    .relation =
        ttx_abstract_same(requirement, value->value_requirement) ||
                ttx_abstract_same(requirement, value->view_bytes_requirement) ||
                ttx_abstract_same(requirement, ttx_bytes_requirement())
            ? TTX_INTERFACE_SATISFIED
            : TTX_INTERFACE_REJECTED,
  };
  state.capability.operations = &state.operations;
  result.operations->answer(result, &state.capability);
}

static void TTX_CALL value_domain(ttx_abstract self, ttx_domain_result result) {
  (void)self;
  result.operations->unknown(result);
}

static void TTX_CALL
    value_callable(ttx_abstract self, ttx_callable_result result) {
  (void)self;
  result.operations->none(result);
}

static void TTX_CALL value_route(ttx_abstract self, ttx_route_result result) {
  (void)self;
  result.operations->none(result);
}

static void TTX_CALL
    value_finite_extent(ttx_abstract self, ttx_finite_extent_result result) {
  (void)self;
  result.operations->none(result);
}

static ttx_abstract bytes_candidate(ttx_bytes view) {
  return value_handle((struct value_state*)view.self);
}

static uint64_t bytes_size(ttx_bytes view) {
  return ((struct value_state*)view.self)->text.size;
}

static void bytes_visit(ttx_bytes view, ttx_bytes_sink result) {
  result.operations->bytes(result, ((struct value_state*)view.self)->text);
  result.operations->completed(result);
}

static void value_bytes(ttx_abstract self, ttx_bytes_result result) {
  result.operations->resolved(
      result, (ttx_bytes){
                .operations = &bytes_operations,
                .self = (ttx_bytes_self*)value_state(self),
              });
}

static const ttx_bytes_ops bytes_operations = {
  .header = ABI_HEADER(ttx_bytes_ops),
  .candidate = bytes_candidate,
  .size = bytes_size,
  .visit = bytes_visit,
};

static void TTX_CALL layout_fit(
    ttx_layout self,
    ttx_pack source,
    ttx_context context,
    ttx_pack_result result) {
  (void)self;
  (void)source;
  (void)context;
  result.operations->none(result);
}

static void TTX_CALL
    layout_enumerable(ttx_layout self, ttx_enumerable_result result) {
  result.operations->satisfied(
      result, value_enumerable((struct value_state*)self.self));
}

static void TTX_CALL layout_named(ttx_layout self, ttx_named_result result) {
  (void)self;
  result.operations->rejected(result);
}

static struct value_snapshot* snapshot_state(ttx_layout_snapshot self) {
  return (struct value_snapshot*)self.self;
}

static ttx_layout TTX_CALL snapshot_layout(ttx_layout_snapshot self) {
  struct value_snapshot* snapshot = snapshot_state(self);
  return snapshot == 0 ? (ttx_layout){0} : value_layout(snapshot->value);
}

static void TTX_CALL snapshot_release(ttx_layout_snapshot self) {
  free(snapshot_state(self));
}

static void TTX_CALL
    layout_snapshot(ttx_layout self, ttx_layout_snapshot_result result) {
  struct value_snapshot* snapshot = malloc(sizeof(struct value_snapshot));
  if (snapshot == 0) {
    result.operations->support_failed(result, TTX_PACK_SUPPORT_EXHAUSTED);
    return;
  }
  snapshot->operations = snapshot_operations;
  snapshot->value = (struct value_state*)self.self;
  const ttx_layout_snapshot retained = {
    .operations = &snapshot->operations,
    .self = (ttx_layout_snapshot_self*)snapshot,
  };
  result.operations->retained(result, retained);
}

static void TTX_CALL layout_fluid(ttx_layout self, ttx_fluid_result result) {
  (void)self;
  result.operations->rejected(result);
}

static void TTX_CALL
    layout_value(ttx_layout self, ttx_value_layout_result result) {
  (void)self;
  result.operations->rejected(result);
}

static void TTX_CALL
    layout_composite(ttx_layout self, ttx_composite_layout_result result) {
  (void)self;
  result.operations->rejected(result);
}

static void TTX_CALL
    layout_ranged(ttx_layout self, ttx_ranged_layout_result result) {
  (void)self;
  result.operations->rejected(result);
}

static void TTX_CALL
    layout_reindexed(ttx_layout self, ttx_reindexed_layout_result result) {
  (void)self;
  result.operations->rejected(result);
}

static ttx_layout TTX_CALL enumerable_layout(ttx_enumerable self) {
  return value_layout((struct value_state*)self.self);
}

static uint64_t TTX_CALL enumerable_cardinality(ttx_enumerable self) {
  (void)self;
  return 1;
}

static void TTX_CALL
    enumerable_visit(ttx_enumerable self, ttx_layout_entry_sink result) {
  static const uint8_t zero[] = "0";
  const ttx_borrowed_bytes path = {zero, 1};
  result.operations->entry(
      result, path, value_handle((struct value_state*)self.self));
  result.operations->completed(result);
}

static const ttx_abstract_ops value_operations = {
  .header = ABI_HEADER(ttx_abstract_ops),
  .name = value_name,
  .documentation = value_documentation,
  .resolve = value_resolve,
  .resolve_concept = value_resolve_concept,
  .visit_concepts = value_visit_concepts,
  .interface = value_interface,
  .resolve_domain = value_domain,
  .resolve_callable = value_callable,
  .resolve_route = value_route,
  .resolve_finite_extent = value_finite_extent,
  .resolve_bytes = value_bytes,
};

static const ttx_layout_ops layout_operations = {
  .header = ABI_HEADER(ttx_layout_ops),
  .fit = layout_fit,
  .enumerable = layout_enumerable,
  .named = layout_named,
  .snapshot = layout_snapshot,
  .fluid = layout_fluid,
  .value = layout_value,
  .composite = layout_composite,
  .ranged = layout_ranged,
  .reindexed = layout_reindexed,
};

static const ttx_enumerable_ops enumerable_operations = {
  .header = ABI_HEADER(ttx_enumerable_ops),
  .layout = enumerable_layout,
  .cardinality = enumerable_cardinality,
  .visit = enumerable_visit,
};

static const ttx_layout_snapshot_ops snapshot_operations = {
  .header = ABI_HEADER(ttx_layout_snapshot_ops),
  .layout = snapshot_layout,
  .release = snapshot_release,
};

ttx_abstract ttx_test_c_value_create(
    ttx_borrowed_bytes name,
    ttx_borrowed_bytes text,
    ttx_abstract selected_value_requirement,
    ttx_abstract selected_view_bytes_requirement,
    ttx_abstract selected_to_string_operation) {
  return register_value(
      name, text, selected_value_requirement, selected_view_bytes_requirement,
      selected_to_string_operation);
}
