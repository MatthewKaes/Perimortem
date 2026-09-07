// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "validation/cross_language/source.h"
#include <stdlib.h>
#include <string.h>

typedef struct Source Source;
typedef struct {
  ttx_abstract_capability capability;
  Source* owner;
  int payload;
} Node;

struct Source {
  size_t references;
  tetrodotoxin_source_input input;
  Node root;
  Node payload;
};

typedef struct {
  ttx_interface_capability capability;
  ttx_abstract requirement;
  ttx_abstract candidate;
  ttx_interface_relation relation;
} Witness;

static uint64_t live_graphs;
static const Node* node(ttx_abstract value) { return (const Node*)value; }
static ttx_borrowed_bytes literal(const char* value) {
  return (ttx_borrowed_bytes){(const uint8_t*)value, strlen(value)};
}
static ttx_abstract witness_requirement(ttx_interface self) { return ((const Witness*)self)->requirement; }
static ttx_abstract witness_candidate(ttx_interface self) { return ((const Witness*)self)->candidate; }
static ttx_interface_relation witness_relation(ttx_interface self) { return ((const Witness*)self)->relation; }
static void witness_invoke(ttx_interface self, ttx_abstract op, ttx_pack in,
    ttx_context context, ttx_pack_result result) {
  (void)self; (void)op; (void)in; (void)context;
  result.operations->none(result);
}
static const ttx_interface_ops witness_ops = {
  .header = {sizeof(ttx_interface_ops), TTX_ABI_MAJOR, TTX_ABI_MINOR},
  .requirement = witness_requirement, .candidate = witness_candidate,
  .negotiate = witness_relation, .invoke = witness_invoke,
};
static void interfaces(ttx_abstract self, ttx_abstract requirement, ttx_interface_sink sink) {
  const Node* n = node(self);
  int accepted = n->owner == NULL
      ? ttx_abstract_same(requirement, tetrodotoxin_dialect_requirement())
      : n->payload && (ttx_abstract_same(requirement, ttx_bytes_requirement()) ||
          ttx_abstract_same(requirement, ttx_constant_requirement()));
  Witness witness = {{&witness_ops}, requirement, self,
      ttx_abstract_same(requirement, self) ? TTX_INTERFACE_EQUIVALENT :
      accepted ? TTX_INTERFACE_SATISFIED : TTX_INTERFACE_REJECTED};
  sink.operations->answer(sink, &witness.capability);
}
static ttx_borrowed_bytes name(ttx_abstract self) {
  return literal(node(self)->owner == NULL ? "C" : node(self)->payload ? "value" : "source");
}
static ttx_documentation documentation(ttx_abstract self) {
  (void)self;
  ttx_abstract unknown = ttx_unknown();
  return unknown->operations->documentation(unknown);
}
static void resolve(ttx_abstract self, ttx_abstract_sink sink) { sink.operations->answer(sink, self); }
static void concept(ttx_abstract self, ttx_borrowed_bytes route, ttx_abstract_sink sink) {
  const Node* n = node(self);
  ttx_abstract answer = ttx_none();
  if (n->owner && !n->payload && route.size == 5 && memcmp(route.data, "value", 5) == 0)
    answer = &n->owner->payload.capability;
  sink.operations->answer(sink, answer);
}
static void concepts(ttx_abstract self, ttx_concept_sink sink) {
  const Node* n = node(self);
  if (n->owner && !n->payload)
    sink.operations->item(sink, literal("value"), &n->owner->payload.capability);
  sink.operations->completed(sink);
}
static void domain(ttx_abstract self, ttx_domain_result result) {
  if (node(self)->payload) result.operations->unknown(result);
  else result.operations->none(result);
}
static void callable(ttx_abstract self, ttx_callable_result result) { (void)self; result.operations->none(result); }
static void route(ttx_abstract self, ttx_route_result result) { (void)self; result.operations->none(result); }
static void extent(ttx_abstract self, ttx_finite_extent_result result) { (void)self; result.operations->none(result); }
static ttx_abstract bytes_candidate(ttx_bytes value) { return &((Node*)value.self)->capability; }
static ttx_borrowed_bytes payload(ttx_bytes value) {
  Source* source = ((Node*)value.self)->owner;
  return source->input.operations->bytes(source->input.self);
}
static uint64_t bytes_size(ttx_bytes value) { return payload(value).size; }
static void bytes_visit(ttx_bytes value, ttx_bytes_sink sink) {
  sink.operations->bytes(sink, payload(value));
  sink.operations->completed(sink);
}
static const ttx_bytes_ops bytes_ops = {
  .header = {sizeof(ttx_bytes_ops), TTX_ABI_MAJOR, TTX_ABI_MINOR},
  .candidate = bytes_candidate, .size = bytes_size, .visit = bytes_visit,
};
static void bytes(ttx_abstract self, ttx_bytes_result result) {
  if (!node(self)->payload) { result.operations->none(result); return; }
  result.operations->resolved(result, (ttx_bytes){&bytes_ops, (ttx_bytes_self*)self});
}
static const ttx_abstract_ops node_ops = {
  .header = {sizeof(ttx_abstract_ops), TTX_ABI_MAJOR, TTX_ABI_MINOR},
  .name = name, .documentation = documentation, .resolve = resolve,
  .resolve_concept = concept, .visit_concepts = concepts, .interface = interfaces,
  .resolve_domain = domain, .resolve_callable = callable, .resolve_route = route,
  .resolve_finite_extent = extent, .resolve_bytes = bytes,
};
static Node provider_node = {{&node_ops}, NULL, 0};
static void retain(tetrodotoxin_source_graph_self* self) { ++((Source*)self)->references; }
static void release(tetrodotoxin_source_graph_self* self) {
  Source* source = (Source*)self;
  if (--source->references == 0) {
    source->input.operations->release(source->input.self);
    --live_graphs;
    free(source);
  }
}
static ttx_abstract root(tetrodotoxin_source_graph_self* self) { return &((Source*)self)->root.capability; }
static ttx_abstract dialect(tetrodotoxin_source_graph_self* self) { (void)self; return &provider_node.capability; }
static void dependencies(tetrodotoxin_source_graph_self* self, tetrodotoxin_source_dependency_sink sink) {
  (void)self; sink.operations->completed(sink.self);
}
static void validate(tetrodotoxin_source_graph_self* self, tetrodotoxin_validation_result result) {
  (void)self; result.operations->accepted(result.self);
}
static void produce(tetrodotoxin_source_graph_self* self, ttx_context context, tetrodotoxin_production_result result) {
  (void)self; (void)context; result.operations->none(result.self);
}
static void associations(tetrodotoxin_source_graph_self* self, tetrodotoxin_source_associations sink) {
  Source* source = (Source*)self;
  const ttx_borrowed_bytes bytes = source->input.operations->bytes(source->input.self);
  sink.operations->association(sink, (tetrodotoxin_source_anchor){0, bytes.size, 0, bytes.size},
      &source->payload.capability);
  sink.operations->completed(sink);
}
static void diagnostics(tetrodotoxin_source_graph_self* self, tetrodotoxin_source_diagnostics sink) {
  (void)self; sink.operations->completed(sink);
}
static const tetrodotoxin_source_graph_ops graph_ops = {
  .header = {sizeof(tetrodotoxin_source_graph_ops), TTX_ABI_MAJOR, TTX_ABI_MINOR},
  .retain = retain, .release = release, .root = root, .dialect = dialect,
  .visit_dependencies = dependencies, .validate = validate, .produce = produce,
  .visit_associations = associations, .visit_diagnostics = diagnostics,
};
static void provider_retain(tetrodotoxin_dialect_provider_self* self) { (void)self; }
static void provider_release(tetrodotoxin_dialect_provider_self* self) { (void)self; }
static ttx_abstract provider_candidate(tetrodotoxin_dialect_provider_self* self) { (void)self; return &provider_node.capability; }
static ttx_borrowed_bytes provider_name(tetrodotoxin_dialect_provider_self* self) { (void)self; return literal("C"); }
static void interpret(tetrodotoxin_dialect_provider_self* self, tetrodotoxin_source_input input,
    ttx_abstract context, tetrodotoxin_interpret_result result) {
  (void)self; (void)context;
  Source* source = malloc(sizeof(*source));
  if (!source) abort();
  source->references = 1;
  source->input = input;
  input.operations->retain(input.self);
  source->root = (Node){{&node_ops}, source, 0};
  source->payload = (Node){{&node_ops}, source, 1};
  ++live_graphs;
  result.operations->constructed(result.self,
      (tetrodotoxin_source_graph){&graph_ops, (tetrodotoxin_source_graph_self*)source});
}
tetrodotoxin_dialect_provider validation_source_provider(void) {
  static const tetrodotoxin_dialect_provider_ops operations = {
    .header = {sizeof(tetrodotoxin_dialect_provider_ops), TTX_ABI_MAJOR, TTX_ABI_MINOR},
    .retain = provider_retain, .release = provider_release,
    .candidate = provider_candidate, .name = provider_name, .interpret = interpret,
  };
  return (tetrodotoxin_dialect_provider){&operations, (tetrodotoxin_dialect_provider_self*)&provider_node};
}
uint64_t validation_source_live_graphs(void) { return live_graphs; }
