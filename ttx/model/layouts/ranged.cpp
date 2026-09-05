// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "ttx/model/layouts/ranged.hpp"

#include <cstddef>
#include <cstdlib>
#include <new>

#include "ttx/concept/fitting.hpp"
#include "ttx/query.hpp"

using namespace Ttx;
using namespace Ttx::Layouts;

struct RangedCapture {
  ttx_ranged_layout_result_ops operations;
  bool answered;
  bool satisfied;
  ttx_ranged_layout ranged;
};

template <typename Operations>
static auto supports(const Operations* operations, uint32_t size) -> bool {
  return operations != nullptr &&
         operations->header.abi_major == TTX_ABI_MAJOR &&
         operations->header.size >= size;
}

static auto select(ttx_ranged_layout_result self) -> RangedCapture& {
  return *reinterpret_cast<RangedCapture*>(self.self);
}

static void TTX_CALL ranged_rejected(ttx_ranged_layout_result self) {
  RangedCapture& capture = select(self);
  capture.answered = true;
  capture.satisfied = false;
}

static void TTX_CALL
    ranged_satisfied(ttx_ranged_layout_result self, ttx_ranged_layout ranged) {
  RangedCapture& capture = select(self);
  capture.answered = true;
  capture.satisfied = true;
  capture.ranged = ranged;
}

static auto query_ranged(ttx_layout layout, ttx_ranged_layout& ranged) -> bool {
  if (!supports(layout.operations, sizeof(ttx_layout_ops))) {
    return false;
  }
  RangedCapture capture = {
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
  };
  const ttx_ranged_layout_result result = {
    .operations = &capture.operations,
    .self = reinterpret_cast<ttx_ranged_layout_result_self*>(&capture),
  };
  layout.operations->ranged(layout, result);
  if (!capture.answered || !capture.satisfied ||
      !supports(capture.ranged.operations, sizeof(ttx_ranged_layout_ops))) {
    return false;
  }
  const ttx_layout candidate =
      capture.ranged.operations->candidate(capture.ranged);
  if (candidate.self != layout.self) {
    return false;
  }
  ranged = capture.ranged;
  return true;
}

Ranged::Ranged(ttx_abstract producer, ttx_abstract extent)
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
      ranged_binding({
        .operations =
            {
              .header =
                  {
                    .size = sizeof(ttx_ranged_layout_ops),
                    .abi_major = TTX_ABI_MAJOR,
                    .abi_minor = TTX_ABI_MINOR,
                  },
              .candidate = ranged_candidate,
              .producer = ranged_producer,
              .extent = ranged_extent,
            },
      }),
      producer(producer),
      extent(extent) {
  if (producer == nullptr || producer->operations == nullptr ||
      extent == nullptr || extent->operations == nullptr ||
      ttx_abstract_same(producer, ttx_none()) ||
      ttx_abstract_same(extent, ttx_none())) {
    std::abort();
  }
}

void Ranged::fit(ttx_pack source, ttx_context context, ttx_pack_result result)
    const {
  if (!supports(source.operations, sizeof(ttx_pack_ops)) ||
      !supports(context.operations, sizeof(ttx_context_ops))) {
    result.operations->support_failed(result, TTX_PACK_SUPPORT_INVALID_LAYOUT);
    return;
  }
  const ExtentObservation receiving_extent = Ttx::resolve_finite_extent(extent);
  if (receiving_extent.state == Observation::Unknown) {
    result.operations->unknown(result);
    return;
  }
  if (receiving_extent.state == Observation::None) {
    result.operations->none(result);
    return;
  }
  const ttx_layout source_layout = source.operations->layout(source);
  ttx_ranged_layout source_ranged;
  if (!query_ranged(source_layout, source_ranged)) {
    result.operations->none(result);
    return;
  }
  const ExtentObservation supplied_extent = Ttx::resolve_finite_extent(
      source_ranged.operations->extent(source_ranged));
  if (supplied_extent.state == Observation::Unknown) {
    result.operations->unknown(result);
    return;
  }
  if (supplied_extent.state == Observation::None ||
      supplied_extent.extent.operations->cardinality(supplied_extent.extent) !=
          receiving_extent.extent.operations->cardinality(
              receiving_extent.extent)) {
    result.operations->none(result);
    return;
  }
  const ttx_abstract supplied =
      source_ranged.operations->producer(source_ranged);
  const LeafFit fit = Ttx::fit_leaf(supplied, producer);
  if (fit == LeafFit::None) {
    result.operations->none(result);
    return;
  }
  Ranged witness(fit == LeafFit::Unknown ? ttx_unknown() : supplied, extent);
  context.operations->pack(context, witness.get_abi(), result);
}

void Ranged::enumerable(ttx_enumerable_result result) const {
  if (Ttx::resolve_finite_extent(extent).state != Observation::Resolved) {
    result.operations->rejected(result);
    return;
  }
  result.operations->satisfied(
      result, {
                .operations = &enumerable_binding.operations,
                .self = reinterpret_cast<ttx_enumerable_self*>(
                    const_cast<Ranged*>(this)),
              });
}

void Ranged::snapshot(ttx_layout_snapshot_result result) const {
  auto* copy = new (std::nothrow) Ranged(producer, extent);
  if (copy == nullptr) {
    result.operations->support_failed(result, TTX_PACK_SUPPORT_EXHAUSTED);
    return;
  }
  result.operations->retained(
      result, {
                .operations = &copy->snapshot_binding.operations,
                .self = reinterpret_cast<ttx_layout_snapshot_self*>(copy),
              });
}

void Ranged::ranged(ttx_ranged_layout_result result) const {
  result.operations->satisfied(
      result, {
                .operations = &ranged_binding.operations,
                .self = reinterpret_cast<ttx_ranged_layout_self*>(
                    const_cast<Ranged*>(this)),
              });
}

template <typename Binding, typename Handle>
static auto select_owner(Handle self) -> Ranged& {
  if (self.operations == nullptr || self.self == nullptr) {
    std::abort();
  }
  return *reinterpret_cast<Ranged*>(self.self);
}

auto Ranged::select(ttx_enumerable self) -> const Ranged& {
  return select_owner<EnumerableBinding>(self);
}

auto Ranged::select(ttx_layout_snapshot self) -> Ranged& {
  return select_owner<SnapshotBinding>(self);
}

auto Ranged::select(ttx_ranged_layout self) -> const Ranged& {
  return select_owner<RangedBinding>(self);
}

auto TTX_CALL Ranged::enumerable_layout(ttx_enumerable self) -> ttx_layout {
  return select(self).get_abi();
}

auto TTX_CALL Ranged::enumerable_cardinality(ttx_enumerable self) -> uint64_t {
  const ExtentObservation extent =
      Ttx::resolve_finite_extent(select(self).extent);
  if (extent.state != Observation::Resolved) {
    std::abort();
  }
  return extent.extent.operations->cardinality(extent.extent);
}

void TTX_CALL Ranged::enumerable_visit(
    ttx_enumerable self,
    ttx_layout_entry_sink result) {
  const Ranged& selected = select(self);
  const uint64_t count = enumerable_cardinality(self);
  for (uint64_t index = 0; index < count; ++index) {
    uint8_t path[8];
    for (uint64_t byte = 0; byte < sizeof(path); ++byte) {
      path[sizeof(path) - byte - 1] = static_cast<uint8_t>(index >> (byte * 8));
    }
    result.operations->entry(result, {path, sizeof(path)}, selected.producer);
  }
  result.operations->completed(result);
}

auto TTX_CALL Ranged::snapshot_layout(ttx_layout_snapshot self) -> ttx_layout {
  return select(self).get_abi();
}

void TTX_CALL Ranged::snapshot_release(ttx_layout_snapshot self) {
  delete &select(self);
}

auto TTX_CALL Ranged::ranged_candidate(ttx_ranged_layout self) -> ttx_layout {
  return select(self).get_abi();
}

auto TTX_CALL Ranged::ranged_producer(ttx_ranged_layout self) -> ttx_abstract {
  return select(self).producer;
}

auto TTX_CALL Ranged::ranged_extent(ttx_ranged_layout self) -> ttx_abstract {
  return select(self).extent;
}
