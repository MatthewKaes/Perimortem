// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/language/product.h"

struct product_requirement {
  struct ttx_abstract abstract;
};

static const uint8_t product_name[] = "Product";

static struct perimortem_bytes name(const struct ttx_abstract* self) {
  struct perimortem_bytes value = {
      .data = product_name,
      .size = sizeof(product_name) - 1,
  };
  (void)self;
  return value;
}

static const struct ttx_documentation* documentation(const struct ttx_abstract* self) {
  (void)self;
  return ttx_documentation_empty();
}

static const struct ttx_abstract* identity(const struct ttx_abstract* self) {
  return self;
}

static const struct ttx_abstract* type(const struct ttx_abstract* self) {
  (void)self;
  return ttx_none();
}

static const struct ttx_abstract* concept(
    const struct ttx_abstract* self,
    struct perimortem_bytes concept_name) {
  (void)self;
  (void)concept_name;
  return ttx_unknown();
}

static void concepts(
    const struct ttx_abstract* self,
    struct ttx_named_abstract_callable* visitor) {
  (void)self;
  (void)visitor;
}

static struct ttx_interface interface(
    const struct ttx_abstract* self,
    const struct ttx_abstract* requirement) {
  return ttx_interface_rejected(requirement, self);
}

static const struct ttx_abstract_operations operations = {
    .name = name,
    .documentation = documentation,
    .resolve = identity,
    .type = type,
    .resolve_concept = concept,
    .visit_concepts = concepts,
    .interface = interface,
};

static const struct product_requirement requirement = {
    .abstract = {.operations = &operations},
};

const struct ttx_abstract* tetrodotoxin_product_requirement(void) {
  return &requirement.abstract;
}

perimortem_bool tetrodotoxin_product_prove(
    const struct ttx_abstract* candidate,
    struct tetrodotoxin_product_view* view) {
  struct ttx_interface relation = ttx_abstract_interface(
      candidate, tetrodotoxin_product_requirement());
  if (!ttx_interface_accepts(&relation)) {
    return PERIMORTEM_FALSE;
  }
  view->identity = candidate;
  view->operations =
      (const struct tetrodotoxin_product_operations*)relation.operations;
  return PERIMORTEM_TRUE;
}

struct perimortem_bytes tetrodotoxin_product_value(
    const struct tetrodotoxin_product_view* product) {
  return product->operations->value(product->identity);
}

ttx_interface_relation tetrodotoxin_product_relation(
    const struct ttx_abstract* requirement_value,
    const struct ttx_abstract* candidate) {
  (void)requirement_value;
  (void)candidate;
  return TTX_INTERFACE_SATISFIED;
}
