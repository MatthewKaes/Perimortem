// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "ttx/model/layouts/addressable.h"

#include <stddef.h>

#include "ttx/concept/none.h"

#define TTX_CONTAINER_OF(pointer, type, member) \
  ((type*)((uint8_t*)(pointer) - offsetof(type, member)))

static struct perimortem_bytes name(const struct ttx_abstract* base) {
  const struct ttx_layout_addressable* self =
      TTX_CONTAINER_OF(base, const struct ttx_layout_addressable, abstract);
  return self->name;
}

static const struct ttx_documentation* documentation(
    const struct ttx_abstract* self) {
  (void)self;
  return ttx_documentation_empty();
}

static const struct ttx_abstract* identity(const struct ttx_abstract* self) {
  return self;
}

static const struct ttx_abstract* type(const struct ttx_abstract* base) {
  const struct ttx_layout_addressable* self =
      TTX_CONTAINER_OF(base, const struct ttx_layout_addressable, abstract);
  return self->type;
}

static const struct ttx_abstract* unknown_concept(
    const struct ttx_abstract* self,
    struct perimortem_bytes concept) {
  (void)self;
  (void)concept;
  return ttx_unknown();
}

static void no_concepts(
    const struct ttx_abstract* self,
    struct ttx_named_abstract_callable* visitor) {
  (void)self;
  (void)visitor;
}

static ttx_interface_relation satisfied(
    const struct ttx_abstract* requirement,
    const struct ttx_abstract* candidate) {
  (void)requirement;
  (void)candidate;
  return TTX_INTERFACE_SATISFIED;
}

static const struct ttx_abstract* addressable_type(
    const struct ttx_abstract* base) {
  return type(base);
}

static struct ttx_interface interface(
    const struct ttx_abstract* base,
    const struct ttx_abstract* requirement) {
  const struct ttx_layout_addressable* self =
      TTX_CONTAINER_OF(base, const struct ttx_layout_addressable, abstract);
  if (requirement == ttx_addressable_requirement()) {
    return ttx_interface_satisfied(
        requirement, base, &self->addressable.interface);
  }

  if (requirement == ttx_abstract_requirement()) {
    return ttx_interface_satisfied(
        requirement, base, ttx_interface_marker_operations());
  }

  return ttx_interface_rejected(requirement, base);
}

static const struct ttx_abstract_operations abstract_operations = {
  .name = name,
  .documentation = documentation,
  .resolve = identity,
  .type = type,
  .resolve_concept = unknown_concept,
  .visit_concepts = no_concepts,
  .interface = interface,
};

void ttx_layout_addressable_initialize(
    struct ttx_layout_addressable* addressable,
    struct perimortem_bytes addressable_name,
    const struct ttx_abstract* addressable_type_value) {
  static const struct ttx_addressable_operations operations = {
    .interface = {.negotiate = satisfied},
    .type = addressable_type,
  };
  addressable->abstract.operations = &abstract_operations;
  addressable->name = addressable_name;
  addressable->type = addressable_type_value;
  addressable->addressable = operations;
}

void ttx_addressable_layout_initialize(
    struct ttx_addressable_layout* layout,
    const struct ttx_abstract* const* entries,
    const struct perimortem_bytes* names,
    perimortem_count count) {
  ttx_fluid_layout_initialize(&layout->entries, entries, count);
  ttx_named_layout_initialize(
      &layout->named, &layout->entries.layout, names, count);
}
