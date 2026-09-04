// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "ttx/named.hpp"

#include <cstddef>
#include <cstdlib>
#include <new>
#include <utility>
#include <vector>

#include "ttx/query.hpp"

using namespace Ttx;
using namespace Ttx::Layouts;

struct SnapshotCapture {
  ttx_layout_snapshot_result_ops operations;
  bool answered;
  bool valid;
  bool retained;
  ttx_layout_snapshot snapshot;
  ttx_pack_support_failure failure;
};

struct EnumerableCapture {
  ttx_enumerable_result_ops operations;
  bool answered;
  bool valid;
  bool satisfied;
  ttx_enumerable enumerable;
};

struct PathEntry {
  std::vector<uint8_t> path;
  ttx_abstract producer;
};

struct PathCapture {
  ttx_layout_entry_sink_ops operations;
  std::vector<PathEntry>* entries;
  uint64_t expected;
  bool completed;
  bool valid;
};

struct RouteVisit {
  ttx_layout_entry_sink_ops operations;
  ttx_named_route_sink result;
  bool completed;
  bool valid;
};

template <typename Operations>
static auto supports(const Operations* operations, uint32_t size) -> bool {
  return operations != nullptr &&
         operations->header.abi_major == TTX_ABI_MAJOR &&
         operations->header.size >= size;
}

static void release(ttx_layout_snapshot snapshot) {
  if (supports(snapshot.operations, sizeof(ttx_layout_snapshot_ops))) {
    snapshot.operations->release(snapshot);
  }
}

static auto select(ttx_layout_snapshot_result self) -> SnapshotCapture& {
  static_assert(offsetof(SnapshotCapture, operations) == 0);
  return *reinterpret_cast<SnapshotCapture*>(
      const_cast<ttx_layout_snapshot_result_ops*>(self.operations));
}

static void TTX_CALL snapshot_retained(
    ttx_layout_snapshot_result self,
    ttx_layout_snapshot snapshot) {
  SnapshotCapture& capture = select(self);
  if (capture.answered) {
    capture.valid = false;
    release(snapshot);
    return;
  }
  capture.answered = true;
  capture.retained = true;
  capture.snapshot = snapshot;
}

static void TTX_CALL snapshot_failed(
    ttx_layout_snapshot_result self,
    ttx_pack_support_failure failure) {
  SnapshotCapture& capture = select(self);
  if (capture.answered) {
    capture.valid = false;
    return;
  }
  capture.answered = true;
  capture.retained = false;
  capture.failure = failure;
}

static auto capture_snapshot(ttx_layout layout) -> SnapshotCapture {
  SnapshotCapture capture = {
    .operations =
        {
          .header =
              {
                .size = sizeof(ttx_layout_snapshot_result_ops),
                .abi_major = TTX_ABI_MAJOR,
                .abi_minor = TTX_ABI_MINOR,
              },
          .retained = snapshot_retained,
          .support_failed = snapshot_failed,
        },
    .answered = false,
    .valid = true,
    .retained = false,
    .snapshot = {},
    .failure = TTX_PACK_SUPPORT_INVALID_LAYOUT,
  };
  if (!supports(layout.operations, sizeof(ttx_layout_ops))) {
    return capture;
  }
  const ttx_layout_snapshot_result result = {
    .operations = &capture.operations,
    .owner = layout.owner,
    .value = layout.value,
  };
  layout.operations->snapshot(layout, result);
  return capture;
}

static auto select(ttx_enumerable_result self) -> EnumerableCapture& {
  static_assert(offsetof(EnumerableCapture, operations) == 0);
  return *reinterpret_cast<EnumerableCapture*>(
      const_cast<ttx_enumerable_result_ops*>(self.operations));
}

static void TTX_CALL enumerable_rejected(ttx_enumerable_result self) {
  EnumerableCapture& capture = select(self);
  if (capture.answered) {
    capture.valid = false;
    return;
  }
  capture.answered = true;
  capture.satisfied = false;
}

static void TTX_CALL enumerable_satisfied(
    ttx_enumerable_result self,
    ttx_enumerable enumerable) {
  EnumerableCapture& capture = select(self);
  if (capture.answered) {
    capture.valid = false;
    return;
  }
  capture.answered = true;
  capture.satisfied = true;
  capture.enumerable = enumerable;
}

static auto query_enumerable(ttx_layout layout, ttx_enumerable& enumerable)
    -> bool {
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
    .valid = true,
    .satisfied = false,
    .enumerable = {},
  };
  const ttx_enumerable_result result = {
    .operations = &capture.operations,
    .owner = layout.owner,
    .value = layout.value,
  };
  if (!supports(layout.operations, sizeof(ttx_layout_ops))) {
    return false;
  }
  layout.operations->enumerable(layout, result);
  const ttx_layout candidate =
      capture.answered && capture.valid && capture.satisfied &&
              supports(
                  capture.enumerable.operations, sizeof(ttx_enumerable_ops))
          ? capture.enumerable.operations->layout(capture.enumerable)
          : ttx_layout{};
  if (!capture.answered || !capture.valid || !capture.satisfied ||
      !supports(capture.enumerable.operations, sizeof(ttx_enumerable_ops)) ||
      candidate.owner != layout.owner || candidate.value != layout.value) {
    return false;
  }
  enumerable = capture.enumerable;
  return true;
}

static auto select(ttx_layout_entry_sink self) -> PathCapture& {
  static_assert(offsetof(PathCapture, operations) == 0);
  return *reinterpret_cast<PathCapture*>(
      const_cast<ttx_layout_entry_sink_ops*>(self.operations));
}

static void TTX_CALL capture_path(
    ttx_layout_entry_sink self,
    ttx_borrowed_bytes path,
    ttx_abstract producer) {
  PathCapture& capture = select(self);
  if (!capture.valid || capture.completed || producer.operations == nullptr ||
      capture.entries->size() == capture.expected ||
      (path.size != 0 && path.data == nullptr)) {
    capture.valid = false;
    return;
  }
  std::vector<uint8_t> retained(path.size);
  for (uint64_t index = 0; index < path.size; ++index) {
    retained[index] = path.data[index];
  }
  for (const PathEntry& entry : *capture.entries) {
    if (entry.path == retained) {
      capture.valid = false;
      return;
    }
  }
  capture.entries->push_back({
    .path = std::move(retained),
    .producer = producer,
  });
}

static void TTX_CALL paths_completed(ttx_layout_entry_sink self) {
  PathCapture& capture = select(self);
  if (capture.completed) {
    capture.valid = false;
    return;
  }
  capture.completed = true;
}

static auto collect_entries(ttx_layout layout, std::vector<PathEntry>& entries)
    -> bool {
  ttx_enumerable enumerable;
  if (layout.operations == nullptr || !query_enumerable(layout, enumerable)) {
    return false;
  }
  const uint64_t cardinality = enumerable.operations->cardinality(enumerable);
  PathCapture capture = {
    .operations =
        {
          .header =
              {
                .size = sizeof(ttx_layout_entry_sink_ops),
                .abi_major = TTX_ABI_MAJOR,
                .abi_minor = TTX_ABI_MINOR,
              },
          .entry = capture_path,
          .completed = paths_completed,
        },
    .entries = &entries,
    .expected = cardinality,
    .completed = false,
    .valid = true,
  };
  const ttx_layout_entry_sink result = {
    .operations = &capture.operations,
    .owner = layout.owner,
    .value = layout.value,
  };
  enumerable.operations->visit(enumerable, result);
  return capture.valid && capture.completed && entries.size() == cardinality;
}

static auto same_bytes(ttx_borrowed_bytes left, ttx_borrowed_bytes right)
    -> bool {
  if (left.size != right.size ||
      (left.size != 0 && (left.data == nullptr || right.data == nullptr))) {
    return false;
  }
  for (uint64_t index = 0; index < left.size; ++index) {
    if (left.data[index] != right.data[index]) {
      return false;
    }
  }
  return true;
}

static auto select_route_visit(ttx_layout_entry_sink self) -> RouteVisit& {
  static_assert(offsetof(RouteVisit, operations) == 0);
  return *reinterpret_cast<RouteVisit*>(
      const_cast<ttx_layout_entry_sink_ops*>(self.operations));
}

static void TTX_CALL visit_route(
    ttx_layout_entry_sink self,
    ttx_borrowed_bytes path,
    ttx_abstract producer) {
  RouteVisit& visit = select_route_visit(self);
  if (!visit.valid || visit.completed ||
      !supports(visit.result.operations, sizeof(ttx_named_route_sink_ops))) {
    visit.valid = false;
    return;
  }
  const RouteObservation route = Ttx::resolve_route(producer);
  if (route.state == Observation::Unknown) {
    visit.result.operations->unknown(visit.result, path);
  } else if (route.state == Observation::None) {
    visit.result.operations->none(visit.result, path);
  } else {
    visit.result.operations->route(visit.result, path, route.bytes);
  }
}

static void TTX_CALL routes_completed(ttx_layout_entry_sink self) {
  RouteVisit& visit = select_route_visit(self);
  if (visit.completed) {
    visit.valid = false;
    return;
  }
  visit.completed = true;
}

Named::Named(ttx_layout source, ttx_layout routes)
    : Named(source, routes, {}, {}, ttx_authority_create(), 1) {}

Named::Named(
    ttx_layout source,
    ttx_layout routes,
    ttx_layout_snapshot source_snapshot,
    ttx_layout_snapshot routes_snapshot,
    uint64_t authority,
    uint64_t value)
    : Layout(authority, value),
      named_binding({
        .operations =
            {
              .header =
                  {
                    .size = sizeof(ttx_named_ops),
                    .abi_major = TTX_ABI_MAJOR,
                    .abi_minor = TTX_ABI_MINOR,
                  },
              .candidate = candidate,
              .source = Named::source,
              .routes = Named::routes,
              .visit_routes = visit_routes,
              .select = select_route,
            },
        .owner = this,
      }),
      snapshot_binding({
        .operations =
            {
              .header =
                  {
                    .size = sizeof(ttx_layout_snapshot_ops),
                    .abi_major = TTX_ABI_MAJOR,
                    .abi_minor = TTX_ABI_MINOR,
                  },
              .layout = snapshot_layout,
              .release = snapshot_release,
            },
        .owner = this,
      }),
      source_layout(source),
      routes_layout(routes),
      source_snapshot(source_snapshot),
      routes_snapshot(routes_snapshot) {}

Named::~Named() {
  release(routes_snapshot);
  release(source_snapshot);
}

auto Named::valid() const -> bool {
  std::vector<PathEntry> source_entries;
  std::vector<PathEntry> route_entries;
  if (!collect_entries(source_layout, source_entries) ||
      !collect_entries(routes_layout, route_entries) ||
      source_entries.size() != route_entries.size()) {
    return false;
  }
  for (const PathEntry& source_entry : source_entries) {
    uint64_t matches = 0;
    for (const PathEntry& route_entry : route_entries) {
      if (source_entry.path == route_entry.path) {
        ++matches;
      }
    }
    if (matches != 1) {
      return false;
    }
  }
  for (uint64_t index = 0; index < route_entries.size(); ++index) {
    const RouteObservation route =
        Ttx::resolve_route(route_entries[index].producer);
    if (route.state != Observation::Resolved) {
      continue;
    }
    for (uint64_t earlier = 0; earlier < index; ++earlier) {
      const RouteObservation previous =
          Ttx::resolve_route(route_entries[earlier].producer);
      if (previous.state == Observation::Resolved &&
          same_bytes(route.bytes, previous.bytes)) {
        return false;
      }
    }
  }
  return true;
}

void Named::enumerable(ttx_enumerable_result result) const {
  if (!valid()) {
    result.operations->rejected(result);
    return;
  }
  source_layout.operations->enumerable(source_layout, result);
}

void Named::named(ttx_named_result result) const {
  if (!valid()) {
    result.operations->rejected(result);
    return;
  }
  const ttx_layout identity = get_abi();
  const ttx_named view = {
    .operations = &named_binding.operations,
    .owner = identity.owner,
    .value = identity.value,
  };
  result.operations->satisfied(result, view);
}

void Named::snapshot(ttx_layout_snapshot_result result) const {
  if (!valid()) {
    result.operations->support_failed(result, TTX_PACK_SUPPORT_INVALID_LAYOUT);
    return;
  }
  const SnapshotCapture source = capture_snapshot(source_layout);
  if (!source.answered || !source.valid || !source.retained ||
      !supports(source.snapshot.operations, sizeof(ttx_layout_snapshot_ops))) {
    result.operations->support_failed(result, source.failure);
    return;
  }
  const SnapshotCapture routes = capture_snapshot(routes_layout);
  if (!routes.answered || !routes.valid || !routes.retained ||
      !supports(routes.snapshot.operations, sizeof(ttx_layout_snapshot_ops))) {
    release(source.snapshot);
    result.operations->support_failed(result, routes.failure);
    return;
  }
  const ttx_layout copied_source =
      source.snapshot.operations->layout(source.snapshot);
  const ttx_layout copied_routes =
      routes.snapshot.operations->layout(routes.snapshot);
  const ttx_layout identity = get_abi();
  auto* copy = new (std::nothrow) Named(
      copied_source, copied_routes, source.snapshot, routes.snapshot,
      identity.owner, identity.value);
  if (copy == nullptr) {
    release(routes.snapshot);
    release(source.snapshot);
    result.operations->support_failed(result, TTX_PACK_SUPPORT_EXHAUSTED);
    return;
  }
  const ttx_layout_snapshot snapshot = {
    .operations = &copy->snapshot_binding.operations,
    .owner = identity.owner,
    .value = identity.value,
  };
  result.operations->retained(result, snapshot);
}

auto Named::select(ttx_named self) -> const Named& {
  static_assert(offsetof(NamedBinding, operations) == 0);
  const auto& selected =
      *reinterpret_cast<const NamedBinding*>(self.operations);
  if (selected.owner == nullptr) {
    std::abort();
  }
  const ttx_layout identity = selected.owner->get_abi();
  if (identity.owner != self.owner || identity.value != self.value) {
    std::abort();
  }
  return *selected.owner;
}

auto Named::select(ttx_layout_snapshot self) -> Named& {
  static_assert(offsetof(SnapshotBinding, operations) == 0);
  auto& selected = *reinterpret_cast<SnapshotBinding*>(
      const_cast<ttx_layout_snapshot_ops*>(self.operations));
  if (selected.owner == nullptr) {
    std::abort();
  }
  const ttx_layout identity = selected.owner->get_abi();
  if (identity.owner != self.owner || identity.value != self.value) {
    std::abort();
  }
  return *selected.owner;
}

auto TTX_CALL Named::candidate(ttx_named self) -> ttx_layout {
  return select(self).get_abi();
}

auto TTX_CALL Named::source(ttx_named self) -> ttx_layout {
  return select(self).source_layout;
}

auto TTX_CALL Named::routes(ttx_named self) -> ttx_layout {
  return select(self).routes_layout;
}

void TTX_CALL Named::visit_routes(ttx_named self, ttx_named_route_sink result) {
  const Named& selected = select(self);
  ttx_enumerable enumerable;
  if (!supports(result.operations, sizeof(ttx_named_route_sink_ops)) ||
      !selected.valid() ||
      !query_enumerable(selected.routes_layout, enumerable)) {
    if (supports(result.operations, sizeof(ttx_named_route_sink_ops))) {
      result.operations->completed(result);
    }
    return;
  }
  RouteVisit visit = {
    .operations =
        {
          .header =
              {
                .size = sizeof(ttx_layout_entry_sink_ops),
                .abi_major = TTX_ABI_MAJOR,
                .abi_minor = TTX_ABI_MINOR,
              },
          .entry = visit_route,
          .completed = routes_completed,
        },
    .result = result,
    .completed = false,
    .valid = true,
  };
  const ttx_layout_entry_sink visitor = {
    .operations = &visit.operations,
    .owner = self.owner,
    .value = self.value,
  };
  enumerable.operations->visit(enumerable, visitor);
  result.operations->completed(result);
}

void TTX_CALL Named::select_route(
    ttx_named self,
    ttx_borrowed_bytes route,
    ttx_named_selection_result result) {
  const Named& selected = select(self);
  if (!supports(result.operations, sizeof(ttx_named_selection_result_ops)) ||
      (route.size != 0 && route.data == nullptr) || !selected.valid()) {
    if (supports(result.operations, sizeof(ttx_named_selection_result_ops))) {
      result.operations->unknown(result);
    }
    return;
  }

  std::vector<PathEntry> routes;
  if (!collect_entries(selected.routes_layout, routes)) {
    result.operations->unknown(result);
    return;
  }
  const PathEntry* match = nullptr;
  for (const PathEntry& entry : routes) {
    const RouteObservation observed = Ttx::resolve_route(entry.producer);
    if (observed.state == Observation::Unknown) {
      result.operations->unknown(result);
      return;
    }
    if (observed.state == Observation::Resolved &&
        same_bytes(observed.bytes, route)) {
      if (match != nullptr) {
        result.operations->unknown(result);
        return;
      }
      match = &entry;
    }
  }
  if (match == nullptr) {
    result.operations->none(result);
    return;
  }
  result.operations->selected(
      result, {
                .data = match->path.data(),
                .size = match->path.size(),
              });
}

auto TTX_CALL Named::snapshot_layout(ttx_layout_snapshot self) -> ttx_layout {
  return select(self).get_abi();
}

void TTX_CALL Named::snapshot_release(ttx_layout_snapshot self) {
  delete &select(self);
}
