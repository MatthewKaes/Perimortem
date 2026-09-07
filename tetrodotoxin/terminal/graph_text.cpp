// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/terminal/graph_text.hpp"

#include <algorithm>
#include <cstddef>
#include <deque>
#include <string>
#include <utility>
#include <vector>

#include "ttx/query.hpp"

using namespace Tetrodotoxin::Terminal;

using Bytes = std::vector<uint8_t>;

struct ConceptEdge {
  Bytes route;
  ttx_abstract answer;
};

struct LayoutEntry {
  Bytes path;
  ttx_abstract producer;
};

enum class RouteState {
  Unknown,
  None,
  Exact,
};

struct NamedRoute {
  Bytes path;
  RouteState state;
  Bytes route;
};

struct ReindexMapping {
  Bytes output;
  Bytes source;
};

struct DocumentationCapture {
  ttx_bytes_sink_ops operations;
  std::vector<Bytes> lines;
  bool completed;
};

struct ConceptCapture {
  ttx_concept_sink_ops operations;
  std::vector<ConceptEdge> edges;
  bool completed;
};

struct EnumerableCapture {
  ttx_enumerable_result_ops operations;
  bool answered;
  bool satisfied;
  ttx_enumerable enumerable;
};

struct EntryCapture {
  ttx_layout_entry_sink_ops operations;
  std::vector<LayoutEntry> entries;
  bool completed;
};

struct FluidCapture {
  ttx_fluid_result_ops operations;
  bool answered;
  bool satisfied;
};

struct ValueCapture {
  ttx_value_layout_result_ops operations;
  bool answered;
  bool satisfied;
  ttx_value_layout value;
};

struct CompositeCapture {
  ttx_composite_layout_result_ops operations;
  bool answered;
  bool satisfied;
  ttx_composite_layout composite;
};

struct RangedCapture {
  ttx_ranged_layout_result_ops operations;
  bool answered;
  bool satisfied;
  ttx_ranged_layout ranged;
};

struct NamedCapture {
  ttx_named_result_ops operations;
  bool answered;
  bool satisfied;
  ttx_named named;
};

struct NamedRouteCapture {
  ttx_named_route_sink_ops operations;
  std::vector<NamedRoute> routes;
  bool completed;
};

struct ReindexedCapture {
  ttx_reindexed_layout_result_ops operations;
  bool answered;
  bool satisfied;
  ttx_reindexed_layout reindexed;
};

struct MappingCapture {
  ttx_reindex_sink_ops operations;
  std::vector<ReindexMapping> mappings;
  bool completed;
};

// Graph discovery finishes after the query callbacks return. Keep the shapes
// needed by that later traversal, but leave their producer identities borrowed
// from the graph supplied to this Terminal invocation.
struct DomainShape {
  ttx_abstract domain = ttx_unknown();
  ttx_layout_snapshot snapshot = {};
  Ttx::Observation state = Ttx::Observation::Unknown;
  bool failed = false;

  DomainShape() = default;
  DomainShape(const DomainShape&) = delete;
  DomainShape(DomainShape&& other) noexcept
      : domain(other.domain),
        snapshot(std::exchange(other.snapshot, {})),
        state(other.state),
        failed(other.failed) {}
  ~DomainShape() {
    if (snapshot.operations) {
      snapshot.operations->release(snapshot);
    }
  }

  auto layout() const -> ttx_layout {
    return snapshot.operations ? snapshot.operations->layout(snapshot)
                               : ttx_layout{};
  }
};

static void TTX_CALL domain_shape_retained(
    ttx_layout_snapshot_result result,
    ttx_layout_snapshot snapshot) {
  auto& shape = *reinterpret_cast<DomainShape*>(result.self);
  if (shape.snapshot.operations) {
    shape.failed = true;
    if (snapshot.operations) {
      snapshot.operations->release(snapshot);
    }
    return;
  }
  shape.snapshot = snapshot;
}

static void TTX_CALL domain_shape_failed(
    ttx_layout_snapshot_result result,
    ttx_pack_support_failure) {
  reinterpret_cast<DomainShape*>(result.self)->failed = true;
}

static void TTX_CALL domain_unknown(ttx_domain_result result) {
  reinterpret_cast<DomainShape*>(result.self)->state =
      Ttx::Observation::Unknown;
}

static void TTX_CALL domain_none(ttx_domain_result result) {
  reinterpret_cast<DomainShape*>(result.self)->state = Ttx::Observation::None;
}

static void TTX_CALL domain_resolved(
    ttx_domain_result result,
    ttx_abstract domain,
    ttx_layout layout) {
  auto& shape = *reinterpret_cast<DomainShape*>(result.self);
  shape.state = Ttx::Observation::Resolved;
  shape.domain = domain;
  if (layout.operations == nullptr ||
      layout.operations->header.abi_major != TTX_ABI_MAJOR ||
      layout.operations->header.size < sizeof(ttx_layout_ops) ||
      layout.operations->snapshot == nullptr) {
    shape.failed = true;
    return;
  }
  static const ttx_layout_snapshot_result_ops operations = {
    .header =
        {sizeof(ttx_layout_snapshot_result_ops), TTX_ABI_MAJOR, TTX_ABI_MINOR},
    .retained = domain_shape_retained,
    .support_failed = domain_shape_failed,
  };
  layout.operations->snapshot(
      layout,
      {
        .operations = &operations,
        .self = reinterpret_cast<ttx_layout_snapshot_result_self*>(&shape),
      });
  shape.failed |= shape.snapshot.operations == nullptr;
}

struct Node {
  ttx_abstract abstract;
  Bytes name;
  std::vector<Bytes> documentation;
  ttx_abstract resolved;
  std::vector<ConceptEdge> concepts;
  DomainShape domain;
  Ttx::CallableObservation callable;
  Ttx::RouteObservation route;
  Ttx::ExtentObservation extent;
  ttx_interface_relation constant;
  ttx_interface_relation addressable;
};

struct LayoutNode {
  ttx_layout layout;
  bool enumerable;
  uint64_t cardinality;
  std::vector<LayoutEntry> entries;
  bool fluid;
  bool value;
  ttx_abstract value_producer;
  bool composite;
  ttx_layout left;
  ttx_layout right;
  bool ranged;
  ttx_abstract ranged_producer;
  ttx_abstract ranged_extent;
  bool named;
  ttx_layout named_source;
  ttx_layout named_routes;
  std::vector<NamedRoute> routes;
  bool reindexed;
  ttx_layout reindexed_source;
  ttx_layout reindexed_projection;
  std::vector<ReindexMapping> mappings;
};

template <typename Operations>
static auto supports(const Operations* operations, uint32_t size) -> bool {
  return operations != nullptr &&
         operations->header.abi_major == TTX_ABI_MAJOR &&
         operations->header.size >= size;
}

template <typename Owner, typename Self>
static auto select_owner(Self* self) -> Owner& {
  return *reinterpret_cast<Owner*>(self);
}

static auto copy(ttx_borrowed_bytes value) -> Bytes {
  if (value.size == 0 || value.data == nullptr) {
    return {};
  }
  return Bytes(value.data, value.data + value.size);
}

static auto same(ttx_layout left, ttx_layout right) -> bool {
  // One support owner may expose different views over the same private state.
  // Deduplicate only the complete borrowed handle within this report.
  return left.self == right.self && left.operations == right.operations;
}

static void TTX_CALL
    documentation_bytes(ttx_bytes_sink self, ttx_borrowed_bytes value) {
  select_owner<DocumentationCapture>(self.self).lines.push_back(copy(value));
}

static void TTX_CALL documentation_completed(ttx_bytes_sink self) {
  select_owner<DocumentationCapture>(self.self).completed = true;
}

static auto observe_documentation(ttx_documentation documentation)
    -> std::vector<Bytes> {
  DocumentationCapture capture = {
    .operations =
        {
          .header =
              {
                .size = sizeof(ttx_bytes_sink_ops),
                .abi_major = TTX_ABI_MAJOR,
                .abi_minor = TTX_ABI_MINOR,
              },
          .bytes = documentation_bytes,
          .completed = documentation_completed,
        },
    .lines = {},
    .completed = false,
  };
  if (!supports(documentation.operations, sizeof(ttx_documentation_ops))) {
    return {};
  }
  const ttx_bytes_sink sink = {
    .operations = &capture.operations,
    .self = reinterpret_cast<ttx_bytes_sink_self*>(&capture),
  };
  documentation.operations->visit_bytes(documentation, sink);
  return capture.completed ? std::move(capture.lines) : std::vector<Bytes>{};
}

static void TTX_CALL concept_item(
    ttx_concept_sink self,
    ttx_borrowed_bytes route,
    ttx_abstract answer) {
  select_owner<ConceptCapture>(self.self).edges.push_back(
      {.route = copy(route), .answer = answer});
}

static void TTX_CALL concept_completed(ttx_concept_sink self) {
  select_owner<ConceptCapture>(self.self).completed = true;
}

static auto observe_concepts(ttx_abstract abstract)
    -> std::vector<ConceptEdge> {
  ConceptCapture capture = {
    .operations =
        {
          .header =
              {
                .size = sizeof(ttx_concept_sink_ops),
                .abi_major = TTX_ABI_MAJOR,
                .abi_minor = TTX_ABI_MINOR,
              },
          .item = concept_item,
          .completed = concept_completed,
        },
    .edges = {},
    .completed = false,
  };
  const ttx_concept_sink sink = {
    .operations = &capture.operations,
    .self = reinterpret_cast<ttx_concept_sink_self*>(&capture),
  };
  abstract->operations->visit_concepts(abstract, sink);
  if (!capture.completed) {
    return {};
  }
  std::sort(
      capture.edges.begin(), capture.edges.end(),
      [](const ConceptEdge& left, const ConceptEdge& right) {
        return left.route < right.route;
      });
  return std::move(capture.edges);
}

static void TTX_CALL enumerable_rejected(ttx_enumerable_result self) {
  auto& capture = select_owner<EnumerableCapture>(self.self);
  capture.answered = true;
  capture.satisfied = false;
}

static void TTX_CALL enumerable_satisfied(
    ttx_enumerable_result self,
    ttx_enumerable enumerable) {
  auto& capture = select_owner<EnumerableCapture>(self.self);
  capture.answered = true;
  capture.satisfied = true;
  capture.enumerable = enumerable;
}

static auto observe_enumerable(ttx_layout layout) -> EnumerableCapture {
  EnumerableCapture capture = {
    .operations =
        {
          .header =
              {
                .size = sizeof(ttx_enumerable_result_ops),
                .abi_major = TTX_ABI_MAJOR,
                .abi_minor = TTX_ABI_MINOR,
              },
          .rejected = enumerable_rejected,
          .satisfied = enumerable_satisfied,
        },
    .answered = false,
    .satisfied = false,
    .enumerable = {},
  };
  const ttx_enumerable_result result = {
    .operations = &capture.operations,
    .self = reinterpret_cast<ttx_enumerable_result_self*>(&capture),
  };
  layout.operations->enumerable(layout, result);
  return capture;
}

static void TTX_CALL entry_item(
    ttx_layout_entry_sink self,
    ttx_borrowed_bytes path,
    ttx_abstract producer) {
  select_owner<EntryCapture>(self.self).entries.push_back(
      {.path = copy(path), .producer = producer});
}

static void TTX_CALL entries_completed(ttx_layout_entry_sink self) {
  select_owner<EntryCapture>(self.self).completed = true;
}

static auto observe_entries(ttx_enumerable enumerable)
    -> std::vector<LayoutEntry> {
  EntryCapture capture = {
    .operations =
        {
          .header =
              {
                .size = sizeof(ttx_layout_entry_sink_ops),
                .abi_major = TTX_ABI_MAJOR,
                .abi_minor = TTX_ABI_MINOR,
              },
          .entry = entry_item,
          .completed = entries_completed,
        },
    .entries = {},
    .completed = false,
  };
  const ttx_layout_entry_sink sink = {
    .operations = &capture.operations,
    .self = reinterpret_cast<ttx_layout_entry_sink_self*>(&capture),
  };
  enumerable.operations->visit(enumerable, sink);
  if (!capture.completed) {
    return {};
  }
  std::sort(
      capture.entries.begin(), capture.entries.end(),
      [](const LayoutEntry& left, const LayoutEntry& right) {
        return left.path < right.path;
      });
  return std::move(capture.entries);
}

#define TTX_GRAPH_LAYOUT_QUERY(                                               \
    prefix, capture_type, result_type, view_type, member)                     \
  static void TTX_CALL prefix##_rejected(result_type self) {                  \
    auto& capture = select_owner<capture_type>(self.self);                    \
    capture.answered = true;                                                  \
    capture.satisfied = false;                                                \
  }                                                                           \
  static void TTX_CALL prefix##_satisfied(result_type self, view_type view) { \
    auto& capture = select_owner<capture_type>(self.self);                    \
    capture.answered = true;                                                  \
    capture.satisfied = true;                                                 \
    capture.member = view;                                                    \
  }

TTX_GRAPH_LAYOUT_QUERY(
    value,
    ValueCapture,
    ttx_value_layout_result,
    ttx_value_layout,
    value)
TTX_GRAPH_LAYOUT_QUERY(
    composite,
    CompositeCapture,
    ttx_composite_layout_result,
    ttx_composite_layout,
    composite)
TTX_GRAPH_LAYOUT_QUERY(
    ranged,
    RangedCapture,
    ttx_ranged_layout_result,
    ttx_ranged_layout,
    ranged)
TTX_GRAPH_LAYOUT_QUERY(named, NamedCapture, ttx_named_result, ttx_named, named)
TTX_GRAPH_LAYOUT_QUERY(
    reindexed,
    ReindexedCapture,
    ttx_reindexed_layout_result,
    ttx_reindexed_layout,
    reindexed)

#undef TTX_GRAPH_LAYOUT_QUERY

static void TTX_CALL fluid_rejected(ttx_fluid_result self) {
  auto& capture = select_owner<FluidCapture>(self.self);
  capture.answered = true;
  capture.satisfied = false;
}

static void TTX_CALL fluid_satisfied(ttx_fluid_result self, ttx_fluid) {
  auto& capture = select_owner<FluidCapture>(self.self);
  capture.answered = true;
  capture.satisfied = true;
}

template <typename Capture, typename Result, typename Query>
static auto query_layout(ttx_layout layout, Capture capture, Query query)
    -> Capture {
  using Self = decltype(Result{}.self);
  const Result result = {
    .operations = &capture.operations,
    .self = reinterpret_cast<Self>(&capture),
  };
  query(layout, result);
  return capture;
}

static auto observe_fluid(ttx_layout layout) -> FluidCapture {
  return query_layout<FluidCapture, ttx_fluid_result>(
      layout,
      {
        .operations =
            {
              .header =
                  {
                    .size = sizeof(ttx_fluid_result_ops),
                    .abi_major = TTX_ABI_MAJOR,
                    .abi_minor = TTX_ABI_MINOR,
                  },
              .rejected = fluid_rejected,
              .satisfied = fluid_satisfied,
            },
        .answered = false,
        .satisfied = false,
      },
      layout.operations->fluid);
}

static auto observe_value(ttx_layout layout) -> ValueCapture {
  return query_layout<ValueCapture, ttx_value_layout_result>(
      layout,
      {
        .operations =
            {
              .header =
                  {
                    .size = sizeof(ttx_value_layout_result_ops),
                    .abi_major = TTX_ABI_MAJOR,
                    .abi_minor = TTX_ABI_MINOR,
                  },
              .rejected = value_rejected,
              .satisfied = value_satisfied,
            },
        .answered = false,
        .satisfied = false,
        .value = {},
      },
      layout.operations->value);
}

static auto observe_composite(ttx_layout layout) -> CompositeCapture {
  return query_layout<CompositeCapture, ttx_composite_layout_result>(
      layout,
      {
        .operations =
            {
              .header =
                  {
                    .size = sizeof(ttx_composite_layout_result_ops),
                    .abi_major = TTX_ABI_MAJOR,
                    .abi_minor = TTX_ABI_MINOR,
                  },
              .rejected = composite_rejected,
              .satisfied = composite_satisfied,
            },
        .answered = false,
        .satisfied = false,
        .composite = {},
      },
      layout.operations->composite);
}

static auto observe_ranged(ttx_layout layout) -> RangedCapture {
  return query_layout<RangedCapture, ttx_ranged_layout_result>(
      layout,
      {
        .operations =
            {
              .header =
                  {
                    .size = sizeof(ttx_ranged_layout_result_ops),
                    .abi_major = TTX_ABI_MAJOR,
                    .abi_minor = TTX_ABI_MINOR,
                  },
              .rejected = ranged_rejected,
              .satisfied = ranged_satisfied,
            },
        .answered = false,
        .satisfied = false,
        .ranged = {},
      },
      layout.operations->ranged);
}

static auto observe_named(ttx_layout layout) -> NamedCapture {
  return query_layout<NamedCapture, ttx_named_result>(
      layout,
      {
        .operations =
            {
              .header =
                  {
                    .size = sizeof(ttx_named_result_ops),
                    .abi_major = TTX_ABI_MAJOR,
                    .abi_minor = TTX_ABI_MINOR,
                  },
              .rejected = named_rejected,
              .satisfied = named_satisfied,
            },
        .answered = false,
        .satisfied = false,
        .named = {},
      },
      layout.operations->named);
}

static auto observe_reindexed(ttx_layout layout) -> ReindexedCapture {
  return query_layout<ReindexedCapture, ttx_reindexed_layout_result>(
      layout,
      {
        .operations =
            {
              .header =
                  {
                    .size = sizeof(ttx_reindexed_layout_result_ops),
                    .abi_major = TTX_ABI_MAJOR,
                    .abi_minor = TTX_ABI_MINOR,
                  },
              .rejected = reindexed_rejected,
              .satisfied = reindexed_satisfied,
            },
        .answered = false,
        .satisfied = false,
        .reindexed = {},
      },
      layout.operations->reindexed);
}

static auto route_capture(ttx_named_route_sink self) -> NamedRouteCapture& {
  return select_owner<NamedRouteCapture>(self.self);
}

static void TTX_CALL
    named_route_unknown(ttx_named_route_sink self, ttx_borrowed_bytes path) {
  route_capture(self).routes.push_back(
      {.path = copy(path), .state = RouteState::Unknown, .route = {}});
}

static void TTX_CALL
    named_route_none(ttx_named_route_sink self, ttx_borrowed_bytes path) {
  route_capture(self).routes.push_back(
      {.path = copy(path), .state = RouteState::None, .route = {}});
}

static void TTX_CALL named_route_exact(
    ttx_named_route_sink self,
    ttx_borrowed_bytes path,
    ttx_borrowed_bytes route) {
  route_capture(self).routes.push_back(
      {.path = copy(path), .state = RouteState::Exact, .route = copy(route)});
}

static void TTX_CALL named_routes_completed(ttx_named_route_sink self) {
  route_capture(self).completed = true;
}

static auto observe_named_routes(ttx_named named) -> std::vector<NamedRoute> {
  NamedRouteCapture capture = {
    .operations =
        {
          .header =
              {
                .size = sizeof(ttx_named_route_sink_ops),
                .abi_major = TTX_ABI_MAJOR,
                .abi_minor = TTX_ABI_MINOR,
              },
          .unknown = named_route_unknown,
          .none = named_route_none,
          .route = named_route_exact,
          .completed = named_routes_completed,
        },
    .routes = {},
    .completed = false,
  };
  const ttx_named_route_sink sink = {
    .operations = &capture.operations,
    .self = reinterpret_cast<ttx_named_route_sink_self*>(&capture),
  };
  named.operations->visit_routes(named, sink);
  if (!capture.completed) {
    return {};
  }
  std::sort(
      capture.routes.begin(), capture.routes.end(),
      [](const NamedRoute& left, const NamedRoute& right) {
        return left.path < right.path;
      });
  return std::move(capture.routes);
}

static void TTX_CALL reindex_mapping(
    ttx_reindex_sink self,
    ttx_borrowed_bytes output,
    ttx_borrowed_bytes source) {
  select_owner<MappingCapture>(self.self).mappings.push_back(
      {.output = copy(output), .source = copy(source)});
}

static void TTX_CALL reindex_completed(ttx_reindex_sink self) {
  select_owner<MappingCapture>(self.self).completed = true;
}

static auto observe_mappings(ttx_reindexed_layout reindexed)
    -> std::vector<ReindexMapping> {
  MappingCapture capture = {
    .operations =
        {
          .header =
              {
                .size = sizeof(ttx_reindex_sink_ops),
                .abi_major = TTX_ABI_MAJOR,
                .abi_minor = TTX_ABI_MINOR,
              },
          .mapping = reindex_mapping,
          .completed = reindex_completed,
        },
    .mappings = {},
    .completed = false,
  };
  const ttx_reindex_sink sink = {
    .operations = &capture.operations,
    .self = reinterpret_cast<ttx_reindex_sink_self*>(&capture),
  };
  reindexed.operations->visit_mappings(reindexed, sink);
  if (!capture.completed) {
    return {};
  }
  std::sort(
      capture.mappings.begin(), capture.mappings.end(),
      [](const ReindexMapping& left, const ReindexMapping& right) {
        return left.output < right.output;
      });
  return std::move(capture.mappings);
}

class Observation {
 public:
  auto retain(ttx_abstract abstract) -> uint64_t {
    if (abstract == nullptr ||
        !supports(abstract->operations, sizeof(ttx_abstract_ops))) {
      abstract = ttx_unknown();
    }
    for (uint64_t index = 0; index < nodes.size(); ++index) {
      if (ttx_abstract_same(nodes[index].abstract, abstract)) {
        return index;
      }
    }
    nodes.push_back({.abstract = abstract});
    return nodes.size() - 1;
  }

  auto retain(ttx_layout layout) -> uint64_t {
    if (!supports(layout.operations, sizeof(ttx_layout_ops))) {
      return UINT64_MAX;
    }
    for (uint64_t index = 0; index < layouts.size(); ++index) {
      if (same(layouts[index].layout, layout)) {
        return index;
      }
    }
    layouts.push_back({.layout = layout});
    return layouts.size() - 1;
  }

  void explore() {
    while (observed_nodes != nodes.size() ||
           observed_layouts != layouts.size()) {
      while (observed_nodes != nodes.size()) {
        observe(nodes[observed_nodes++]);
      }
      while (observed_layouts != layouts.size()) {
        observe(layouts[observed_layouts++]);
      }
    }
  }

  auto node_id(ttx_abstract abstract) const -> uint64_t {
    for (uint64_t index = 0; index < nodes.size(); ++index) {
      if (ttx_abstract_same(nodes[index].abstract, abstract)) {
        return index;
      }
    }
    return UINT64_MAX;
  }

  auto layout_id(ttx_layout layout) const -> uint64_t {
    for (uint64_t index = 0; index < layouts.size(); ++index) {
      if (same(layouts[index].layout, layout)) {
        return index;
      }
    }
    return UINT64_MAX;
  }

  // Observing a node discovers more nodes and Layouts. Keep their addresses
  // stable as the traversal grows so the current observation can finish using
  // the record and its retained projections.
  std::deque<Node> nodes;
  std::deque<LayoutNode> layouts;

 private:
  void observe(Node& node) {
    node.name = copy(node.abstract->operations->name(node.abstract));
    node.documentation = observe_documentation(
        node.abstract->operations->documentation(node.abstract));
    node.resolved = Ttx::resolve(node.abstract);
    node.concepts = observe_concepts(node.abstract);
    static const ttx_domain_result_ops domain_operations = {
      .header = {sizeof(ttx_domain_result_ops), TTX_ABI_MAJOR, TTX_ABI_MINOR},
      .unknown = domain_unknown,
      .none = domain_none,
      .resolved = domain_resolved,
    };
    Ttx::resolve_domain(
        node.abstract,
        {
          .operations = &domain_operations,
          .self = reinterpret_cast<ttx_domain_result_self*>(&node.domain),
        });
    node.callable = Ttx::resolve_callable(node.abstract);
    node.route = Ttx::resolve_route(node.abstract);
    node.extent = Ttx::resolve_finite_extent(node.abstract);
    node.constant = Ttx::relation(node.abstract, ttx_constant_requirement());
    node.addressable =
        Ttx::relation(node.abstract, ttx_addressable_requirement());

    retain(node.resolved);
    for (const ConceptEdge& edge : node.concepts) {
      retain(edge.answer);
    }
    if (node.domain.state == Ttx::Observation::Resolved) {
      retain(node.domain.domain);
      retain(node.domain.layout());
    }
    if (node.callable.state == Ttx::CallableObservationState::Resolved) {
      const ttx_callable callable = node.callable.callable;
      retain(callable.operations->parameters(callable));
      retain(callable.operations->results(callable));
    }
    if (node.route.state == Ttx::Observation::Resolved) {
      retain(node.route.candidate);
    }
    if (node.extent.state == Ttx::Observation::Resolved) {
      retain(node.extent.extent.operations->candidate(node.extent.extent));
    }
  }

  void observe(LayoutNode& node) {
    const EnumerableCapture enumerable = observe_enumerable(node.layout);
    node.enumerable =
        enumerable.answered && enumerable.satisfied &&
        supports(enumerable.enumerable.operations, sizeof(ttx_enumerable_ops));
    if (node.enumerable) {
      node.cardinality =
          enumerable.enumerable.operations->cardinality(enumerable.enumerable);
      node.entries = observe_entries(enumerable.enumerable);
      for (const LayoutEntry& entry : node.entries) {
        retain(entry.producer);
      }
    }

    const FluidCapture fluid = observe_fluid(node.layout);
    node.fluid = fluid.answered && fluid.satisfied;

    const ValueCapture value = observe_value(node.layout);
    node.value = value.answered && value.satisfied &&
                 supports(value.value.operations, sizeof(ttx_value_layout_ops));
    if (node.value) {
      node.value_producer = value.value.operations->producer(value.value);
      retain(node.value_producer);
    }

    const CompositeCapture composite = observe_composite(node.layout);
    node.composite =
        composite.answered && composite.satisfied &&
        supports(
            composite.composite.operations, sizeof(ttx_composite_layout_ops));
    if (node.composite) {
      node.left = composite.composite.operations->left(composite.composite);
      node.right = composite.composite.operations->right(composite.composite);
      retain(node.left);
      retain(node.right);
    }

    const RangedCapture ranged = observe_ranged(node.layout);
    node.ranged =
        ranged.answered && ranged.satisfied &&
        supports(ranged.ranged.operations, sizeof(ttx_ranged_layout_ops));
    if (node.ranged) {
      node.ranged_producer = ranged.ranged.operations->producer(ranged.ranged);
      node.ranged_extent = ranged.ranged.operations->extent(ranged.ranged);
      retain(node.ranged_producer);
      retain(node.ranged_extent);
    }

    const NamedCapture named = observe_named(node.layout);
    node.named = named.answered && named.satisfied &&
                 supports(named.named.operations, sizeof(ttx_named_ops));
    if (node.named) {
      node.named_source = named.named.operations->source(named.named);
      node.named_routes = named.named.operations->routes(named.named);
      node.routes = observe_named_routes(named.named);
      retain(node.named_source);
      retain(node.named_routes);
    }

    const ReindexedCapture reindexed = observe_reindexed(node.layout);
    node.reindexed =
        reindexed.answered && reindexed.satisfied &&
        supports(
            reindexed.reindexed.operations, sizeof(ttx_reindexed_layout_ops));
    if (node.reindexed) {
      node.reindexed_source =
          reindexed.reindexed.operations->source(reindexed.reindexed);
      node.reindexed_projection =
          reindexed.reindexed.operations->projection(reindexed.reindexed);
      node.mappings = observe_mappings(reindexed.reindexed);
      retain(node.reindexed_source);
      retain(node.reindexed_projection);
    }
  }

  uint64_t observed_nodes = 0;
  uint64_t observed_layouts = 0;
};

static void append(std::string& output, const char* value) {
  output.append(value);
}

static void append(std::string& output, uint64_t value) {
  output.append(std::to_string(value));
}

static void append_bytes(std::string& output, const Bytes& value) {
  static constexpr char digits[] = "0123456789abcdef";
  output.push_back('x');
  for (uint8_t byte : value) {
    output.push_back(digits[byte >> 4]);
    output.push_back(digits[byte & 0x0F]);
  }
}

static auto observation_name(Ttx::CallableObservationState state) -> const
    char* {
  switch (state) {
  case Ttx::CallableObservationState::Unknown:
    return "unknown";
  case Ttx::CallableObservationState::None:
    return "none";
  case Ttx::CallableObservationState::Resolved:
    return "resolved";
  case Ttx::CallableObservationState::SupportFailed:
    return "support-failed";
  }
  return "unknown";
}

static auto observation_name(Ttx::Observation state) -> const char* {
  switch (state) {
  case Ttx::Observation::Unknown:
    return "unknown";
  case Ttx::Observation::None:
    return "none";
  case Ttx::Observation::Resolved:
    return "resolved";
  }
  return "unknown";
}

static auto relation_name(ttx_interface_relation relation) -> const char* {
  switch (relation) {
  case TTX_INTERFACE_UNKNOWN:
    return "unknown";
  case TTX_INTERFACE_REJECTED:
    return "rejected";
  case TTX_INTERFACE_SATISFIED:
    return "satisfied";
  case TTX_INTERFACE_EQUIVALENT:
    return "equivalent";
  }
  return "unknown";
}

static void write_node(
    std::string& output,
    const Observation& observation,
    uint64_t id,
    const Node& node) {
  append(output, "node ");
  append(output, id);
  append(output, "\n  name ");
  append_bytes(output, node.name);
  append(output, "\n  resolve ");
  append(output, observation.node_id(node.resolved));
  append(output, "\n  constant ");
  append(output, relation_name(node.constant));
  append(output, "\n  addressable ");
  append(output, relation_name(node.addressable));
  append(output, "\n  domain ");
  append(
      output, node.domain.failed ? "support-failed"
                                 : observation_name(node.domain.state));
  if (node.domain.state == Ttx::Observation::Resolved && !node.domain.failed) {
    append(output, " ");
    append(output, observation.node_id(node.domain.domain));
    append(output, " layout ");
    append(output, observation.layout_id(node.domain.layout()));
  }
  append(output, "\n  callable ");
  append(output, observation_name(node.callable.state));
  if (node.callable.state == Ttx::CallableObservationState::Resolved) {
    const ttx_callable callable = node.callable.callable;
    append(output, " parameters ");
    append(
        output,
        observation.layout_id(callable.operations->parameters(callable)));
    append(output, " results ");
    append(
        output, observation.layout_id(callable.operations->results(callable)));
  }
  append(output, "\n  route ");
  append(output, observation_name(node.route.state));
  if (node.route.state == Ttx::Observation::Resolved) {
    append(output, " ");
    append_bytes(output, copy(node.route.bytes));
  }
  append(output, "\n  extent ");
  append(output, observation_name(node.extent.state));
  if (node.extent.state == Ttx::Observation::Resolved) {
    append(output, " ");
    append(
        output, node.extent.extent.operations->cardinality(node.extent.extent));
  }
  append(output, "\n  documentation ");
  append(output, node.documentation.size());
  append(output, "\n");
  for (const Bytes& line : node.documentation) {
    append(output, "    line ");
    append_bytes(output, line);
    append(output, "\n");
  }
  append(output, "  concepts ");
  append(output, node.concepts.size());
  append(output, "\n");
  for (const ConceptEdge& edge : node.concepts) {
    append(output, "    concept ");
    append_bytes(output, edge.route);
    append(output, " ");
    append(output, observation.node_id(edge.answer));
    append(output, "\n");
  }
  append(output, "end\n");
}

static void write_layout(
    std::string& output,
    const Observation& observation,
    uint64_t id,
    const LayoutNode& layout) {
  append(output, "layout ");
  append(output, id);
  append(output, "\n  enumerable ");
  if (!layout.enumerable) {
    append(output, "rejected\n");
  } else {
    append(output, layout.cardinality);
    append(output, "\n");
    for (const LayoutEntry& entry : layout.entries) {
      append(output, "    entry ");
      append_bytes(output, entry.path);
      append(output, " ");
      append(output, observation.node_id(entry.producer));
      append(output, "\n");
    }
  }
  append(output, layout.fluid ? "  fluid satisfied\n" : "  fluid rejected\n");
  if (layout.value) {
    append(output, "  value ");
    append(output, observation.node_id(layout.value_producer));
    append(output, "\n");
  }
  if (layout.composite) {
    append(output, "  composite ");
    append(output, observation.layout_id(layout.left));
    append(output, " ");
    append(output, observation.layout_id(layout.right));
    append(output, "\n");
  }
  if (layout.ranged) {
    append(output, "  ranged ");
    append(output, observation.node_id(layout.ranged_producer));
    append(output, " ");
    append(output, observation.node_id(layout.ranged_extent));
    append(output, "\n");
  }
  if (layout.named) {
    append(output, "  named ");
    append(output, observation.layout_id(layout.named_source));
    append(output, " ");
    append(output, observation.layout_id(layout.named_routes));
    append(output, "\n");
    for (const NamedRoute& route : layout.routes) {
      append(output, "    route ");
      append_bytes(output, route.path);
      if (route.state == RouteState::Unknown) {
        append(output, " unknown\n");
      } else if (route.state == RouteState::None) {
        append(output, " none\n");
      } else {
        append(output, " exact ");
        append_bytes(output, route.route);
        append(output, "\n");
      }
    }
  }
  if (layout.reindexed) {
    append(output, "  reindexed ");
    append(output, observation.layout_id(layout.reindexed_source));
    append(output, " ");
    append(output, observation.layout_id(layout.reindexed_projection));
    append(output, "\n");
    for (const ReindexMapping& mapping : layout.mappings) {
      append(output, "    mapping ");
      append_bytes(output, mapping.output);
      append(output, " ");
      append_bytes(output, mapping.source);
      append(output, "\n");
    }
  }
  append(output, "end\n");
}

auto GraphText::write(
    ttx_borrowed_bytes source,
    ttx_abstract dialect,
    ttx_abstract root,
    ttx_abstract graph) -> Bytes {
  Observation observation;
  const uint64_t graph_id = observation.retain(graph);
  const uint64_t dialect_id = observation.retain(dialect);
  const uint64_t root_id = observation.retain(root);
  observation.explore();

  std::string output = "ttx.graph 2\nsource ";
  append_bytes(output, copy(source));
  append(output, "\ngraph ");
  append(output, graph_id);
  append(output, "\ndialect ");
  append(output, dialect_id);
  append(output, "\nroot ");
  append(output, root_id);
  append(output, "\nnodes ");
  append(output, observation.nodes.size());
  append(output, "\nlayouts ");
  append(output, observation.layouts.size());
  append(output, "\n");
  for (uint64_t id = 0; id < observation.nodes.size(); ++id) {
    write_node(output, observation, id, observation.nodes[id]);
  }
  for (uint64_t id = 0; id < observation.layouts.size(); ++id) {
    write_layout(output, observation, id, observation.layouts[id]);
  }
  return Bytes(output.begin(), output.end());
}
