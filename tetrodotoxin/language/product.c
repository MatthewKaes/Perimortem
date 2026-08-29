// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/language/product.h"

typedef struct product_requirement {
  ttx_abstract abstract;
} product_requirement;

static const uint8_t product_name[] = "Product";

static perimortem_bytes name(const ttx_abstract* self) {
  perimortem_bytes value = {
      .data = product_name,
      .size = sizeof(product_name) - 1,
  };
  (void)self;
  return value;
}

static const ttx_documentation* documentation(const ttx_abstract* self) {
  (void)self;
  return ttx_documentation_empty();
}

static const ttx_abstract* identity(const ttx_abstract* self) {
  return self;
}

static const ttx_abstract* type(const ttx_abstract* self) {
  (void)self;
  return ttx_none();
}

static const ttx_abstract* concept(
    const ttx_abstract* self,
    perimortem_bytes concept_name) {
  (void)self;
  (void)concept_name;
  return ttx_unknown();
}

static void concepts(
    const ttx_abstract* self,
    ttx_named_abstract_callable* visitor) {
  (void)self;
  (void)visitor;
}

static ttx_interface interface(
    const ttx_abstract* self,
    const ttx_abstract* requirement) {
  return ttx_interface_rejected(requirement, self);
}

static const ttx_abstract_operations operations = {
    .name = name,
    .documentation = documentation,
    .resolve = identity,
    .type = type,
    .resolve_concept = concept,
    .visit_concepts = concepts,
    .interface = interface,
};

static const product_requirement requirement = {
    .abstract = {.operations = &operations},
};

const ttx_abstract* tetrodotoxin_product_requirement(void) {
  return &requirement.abstract;
}

perimortem_bool tetrodotoxin_product_prove(
    const ttx_abstract* candidate,
    tetrodotoxin_product_view* view) {
  ttx_interface relation = ttx_abstract_interface(
      candidate, tetrodotoxin_product_requirement());
  if (!ttx_interface_accepts(&relation)) {
    return PERIMORTEM_FALSE;
  }
  view->identity = candidate;
  view->operations =
      (const tetrodotoxin_product_operations*)relation.operations;
  return PERIMORTEM_TRUE;
}

perimortem_bytes tetrodotoxin_product_value(
    const tetrodotoxin_product_view* product) {
  return product->operations->value(product->identity);
}

ttx_interface_relation tetrodotoxin_product_relation(
    const ttx_abstract* requirement_value,
    const ttx_abstract* candidate) {
  (void)requirement_value;
  (void)candidate;
  return TTX_INTERFACE_SATISFIED;
}
