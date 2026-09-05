// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "ttx/model/layouts/reindexed.hpp"

#include <cstddef>
#include <cstdlib>
#include <new>
#include <utility>

using namespace Ttx;
using namespace Ttx::Layouts;

struct Entry {
  std::vector<uint8_t> path;
  ttx_abstract producer;
};

struct EnumerableCapture {
  ttx_enumerable_result_ops operations;
  bool answered;
  bool valid;
  bool satisfied;
  ttx_enumerable enumerable;
};

struct EntryCapture {
  ttx_layout_entry_sink_ops operations;
  std::vector<Entry>* entries;
  uint64_t expected;
  bool completed;
  bool valid;
};

struct SnapshotCapture {
  ttx_layout_snapshot_result_ops operations;
  bool answered;
  bool valid;
  bool retained;
  ttx_layout_snapshot snapshot;
  ttx_pack_support_failure failure;
};

template <typename Operations>
static auto supports(const Operations* operations, uint32_t size) -> bool {
  return operations != nullptr &&
         operations->header.abi_major == TTX_ABI_MAJOR &&
         operations->header.size >= size;
}

static auto select(ttx_enumerable_result self) -> EnumerableCapture& {
  return *reinterpret_cast<EnumerableCapture*>(self.self);
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
  if (!supports(layout.operations, sizeof(ttx_layout_ops))) {
    return false;
  }
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
    .self = reinterpret_cast<ttx_enumerable_result_self*>(&capture),
  };
  layout.operations->enumerable(layout, result);
  if (!capture.answered || !capture.valid || !capture.satisfied ||
      !supports(capture.enumerable.operations, sizeof(ttx_enumerable_ops))) {
    return false;
  }
  const ttx_layout candidate =
      capture.enumerable.operations->layout(capture.enumerable);
  if (candidate.self != layout.self) {
    return false;
  }
  enumerable = capture.enumerable;
  return true;
}

static auto select(ttx_layout_entry_sink self) -> EntryCapture& {
  return *reinterpret_cast<EntryCapture*>(self.self);
}

static void TTX_CALL capture_entry(
    ttx_layout_entry_sink self,
    ttx_borrowed_bytes path,
    ttx_abstract producer) {
  EntryCapture& capture = select(self);
  if (!capture.valid || capture.completed || producer == nullptr ||
      producer->operations == nullptr ||
      capture.entries->size() == capture.expected ||
      (path.size != 0 && path.data == nullptr)) {
    capture.valid = false;
    return;
  }
  std::vector<uint8_t> copied(path.size);
  for (uint64_t index = 0; index < path.size; ++index) {
    copied[index] = path.data[index];
  }
  for (const Entry& entry : *capture.entries) {
    if (entry.path == copied) {
      capture.valid = false;
      return;
    }
  }
  capture.entries->push_back({
    .path = std::move(copied),
    .producer = producer,
  });
}

static void TTX_CALL capture_completed(ttx_layout_entry_sink self) {
  EntryCapture& capture = select(self);
  if (capture.completed) {
    capture.valid = false;
    return;
  }
  capture.completed = true;
}

static auto collect(ttx_layout layout, std::vector<Entry>& entries) -> bool {
  ttx_enumerable enumerable;
  if (!query_enumerable(layout, enumerable)) {
    return false;
  }
  const uint64_t cardinality = enumerable.operations->cardinality(enumerable);
  EntryCapture capture = {
    .operations =
        {
          .header =
              {
                .size = sizeof(ttx_layout_entry_sink_ops),
                .abi_major = TTX_ABI_MAJOR,
                .abi_minor = TTX_ABI_MINOR,
              },
          .entry = capture_entry,
          .completed = capture_completed,
        },
    .entries = &entries,
    .expected = cardinality,
    .completed = false,
    .valid = true,
  };
  const ttx_layout_entry_sink result = {
    .operations = &capture.operations,
    .self = reinterpret_cast<ttx_layout_entry_sink_self*>(&capture),
  };
  enumerable.operations->visit(enumerable, result);
  return capture.valid && capture.completed && entries.size() == cardinality;
}

static auto select(ttx_layout_snapshot_result self) -> SnapshotCapture& {
  return *reinterpret_cast<SnapshotCapture*>(self.self);
}

static void release(ttx_layout_snapshot snapshot) {
  if (supports(snapshot.operations, sizeof(ttx_layout_snapshot_ops))) {
    snapshot.operations->release(snapshot);
  }
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
    .self = reinterpret_cast<ttx_layout_snapshot_result_self*>(&capture),
  };
  layout.operations->snapshot(layout, result);
  return capture;
}

static auto find(
    const std::vector<Entry>& entries,
    const std::vector<uint8_t>& path) -> const Entry* {
  for (const Entry& entry : entries) {
    if (entry.path == path) {
      return &entry;
    }
  }
  return nullptr;
}

Reindexed::Reindexed(
    ttx_layout source,
    ttx_layout projection,
    std::vector<Mapping> mappings)
    : Reindexed(
          source,
          projection,
          std::move(mappings),
          {},
          {}) {}

Reindexed::Reindexed(
    ttx_layout source,
    ttx_layout projection,
    std::vector<Mapping> mappings,
    ttx_layout_snapshot source_snapshot,
    ttx_layout_snapshot projection_snapshot)
    : enumerable_binding({
        .operations =
            {
              .header =
                  {
                    .size = sizeof(ttx_enumerable_ops),
                    .abi_major = TTX_ABI_MAJOR,
                    .abi_minor = TTX_ABI_MINOR,
                  },
              .layout = enumerable_layout,
              .cardinality = enumerable_cardinality,
              .visit = enumerable_visit,
            },
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
      }),
      reindexed_binding({
        .operations =
            {
              .header =
                  {
                    .size = sizeof(ttx_reindexed_layout_ops),
                    .abi_major = TTX_ABI_MAJOR,
                    .abi_minor = TTX_ABI_MINOR,
                  },
              .candidate = reindexed_candidate,
              .source = reindexed_source,
              .projection = reindexed_projection,
              .visit_mappings = visit_mappings,
            },
      }),
      source_layout(source),
      projection_layout(projection),
      mappings(std::move(mappings)),
      source_snapshot(source_snapshot),
      projection_snapshot(projection_snapshot) {}

Reindexed::~Reindexed() {
  release(projection_snapshot);
  release(source_snapshot);
}

auto Reindexed::valid() const -> bool {
  std::vector<Entry> source;
  std::vector<Entry> projection;
  if (!collect(source_layout, source) ||
      !collect(projection_layout, projection) ||
      projection.size() != mappings.size()) {
    return false;
  }
  for (const Mapping& mapping : mappings) {
    const Entry* output = find(projection, mapping.output);
    const Entry* input = find(source, mapping.source);
    if (output == nullptr || input == nullptr ||
        !ttx_abstract_same(output->producer, input->producer)) {
      return false;
    }
  }
  for (const Entry& output : projection) {
    uint64_t matches = 0;
    for (const Mapping& mapping : mappings) {
      if (mapping.output == output.path) {
        ++matches;
      }
    }
    if (matches != 1) {
      return false;
    }
  }
  return true;
}

void Reindexed::fit(
    ttx_pack source,
    ttx_context context,
    ttx_pack_result result) const {
  if (!valid()) {
    result.operations->none(result);
    return;
  }
  projection_layout.operations->fit(projection_layout, source, context, result);
}

void Reindexed::enumerable(ttx_enumerable_result result) const {
  ttx_enumerable projection;
  if (!valid() || !query_enumerable(projection_layout, projection)) {
    result.operations->rejected(result);
    return;
  }
  result.operations->satisfied(
      result, {
                .operations = &enumerable_binding.operations,
                .self = reinterpret_cast<ttx_enumerable_self*>(
                    const_cast<Reindexed*>(this)),
              });
}

void Reindexed::snapshot(ttx_layout_snapshot_result result) const {
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
  const SnapshotCapture projection = capture_snapshot(projection_layout);
  if (!projection.answered || !projection.valid || !projection.retained ||
      !supports(
          projection.snapshot.operations, sizeof(ttx_layout_snapshot_ops))) {
    release(source.snapshot);
    result.operations->support_failed(result, projection.failure);
    return;
  }
  const ttx_layout copied_source =
      source.snapshot.operations->layout(source.snapshot);
  const ttx_layout copied_projection =
      projection.snapshot.operations->layout(projection.snapshot);
  auto* copy = new (std::nothrow) Reindexed(
      copied_source, copied_projection, mappings, source.snapshot,
      projection.snapshot);
  if (copy == nullptr) {
    release(projection.snapshot);
    release(source.snapshot);
    result.operations->support_failed(result, TTX_PACK_SUPPORT_EXHAUSTED);
    return;
  }
  result.operations->retained(
      result, {
                .operations = &copy->snapshot_binding.operations,
                .self = reinterpret_cast<ttx_layout_snapshot_self*>(copy),
              });
}

void Reindexed::reindexed(ttx_reindexed_layout_result result) const {
  if (!valid()) {
    result.operations->rejected(result);
    return;
  }
  result.operations->satisfied(
      result, {
                .operations = &reindexed_binding.operations,
                .self = reinterpret_cast<ttx_reindexed_layout_self*>(
                    const_cast<Reindexed*>(this)),
              });
}

template <typename Binding, typename Handle>
static auto select_owner(Handle self) -> Reindexed& {
  if (self.operations == nullptr || self.self == nullptr) {
    std::abort();
  }
  return *reinterpret_cast<Reindexed*>(self.self);
}

auto Reindexed::select(ttx_enumerable self) -> const Reindexed& {
  return select_owner<EnumerableBinding>(self);
}

auto Reindexed::select(ttx_layout_snapshot self) -> Reindexed& {
  return select_owner<SnapshotBinding>(self);
}

auto Reindexed::select(ttx_reindexed_layout self) -> const Reindexed& {
  return select_owner<ReindexedBinding>(self);
}

auto TTX_CALL Reindexed::enumerable_layout(ttx_enumerable self) -> ttx_layout {
  return select(self).get_abi();
}

auto TTX_CALL Reindexed::enumerable_cardinality(ttx_enumerable self)
    -> uint64_t {
  ttx_enumerable projection;
  if (!query_enumerable(select(self).projection_layout, projection)) {
    std::abort();
  }
  return projection.operations->cardinality(projection);
}

void TTX_CALL Reindexed::enumerable_visit(
    ttx_enumerable self,
    ttx_layout_entry_sink result) {
  ttx_enumerable projection;
  if (!query_enumerable(select(self).projection_layout, projection)) {
    result.operations->completed(result);
    return;
  }
  projection.operations->visit(projection, result);
}

auto TTX_CALL Reindexed::snapshot_layout(ttx_layout_snapshot self)
    -> ttx_layout {
  return select(self).get_abi();
}

void TTX_CALL Reindexed::snapshot_release(ttx_layout_snapshot self) {
  delete &select(self);
}

auto TTX_CALL Reindexed::reindexed_candidate(ttx_reindexed_layout self)
    -> ttx_layout {
  return select(self).get_abi();
}

auto TTX_CALL Reindexed::reindexed_source(ttx_reindexed_layout self)
    -> ttx_layout {
  return select(self).source_layout;
}

auto TTX_CALL Reindexed::reindexed_projection(ttx_reindexed_layout self)
    -> ttx_layout {
  return select(self).projection_layout;
}

void TTX_CALL Reindexed::visit_mappings(
    ttx_reindexed_layout self,
    ttx_reindex_sink result) {
  for (const Mapping& mapping : select(self).mappings) {
    result.operations->mapping(
        result,
        {
          .data = mapping.output.data(),
          .size = mapping.output.size(),
        },
        {
          .data = mapping.source.data(),
          .size = mapping.source.size(),
        });
  }
  result.operations->completed(result);
}
