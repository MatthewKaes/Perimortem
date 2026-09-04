// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "ttx/composite.hpp"

#include <cstddef>
#include <cstdlib>
#include <limits>
#include <new>
#include <vector>

#include "ttx/query.hpp"

using namespace Ttx;
using namespace Ttx::Layouts;

struct EnumerableCapture {
  ttx_enumerable_result_ops operations;
  bool answered;
  bool valid;
  bool satisfied;
  ttx_enumerable enumerable;
};

struct SnapshotCapture {
  ttx_layout_snapshot_result_ops operations;
  bool answered;
  bool valid;
  bool retained;
  ttx_layout_snapshot snapshot;
  ttx_pack_support_failure failure;
};

struct CompositeCapture {
  ttx_composite_layout_result_ops operations;
  bool answered;
  bool satisfied;
  ttx_composite_layout composite;
};

struct VisitFrame {
  ttx_layout_entry_sink_ops operations;
  ttx_layout_entry_sink result;
  uint8_t prefix;
  bool completed;
  bool valid;
};

struct BorrowedPack {
  ttx_pack_ops operations;
  ttx_layout layout;
};

template <typename Operations>
static auto supports(const Operations* operations, uint32_t size) -> bool {
  return operations != nullptr &&
         operations->header.abi_major == TTX_ABI_MAJOR &&
         operations->header.size >= size;
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
    .owner = layout.owner,
    .value = layout.value,
  };
  layout.operations->enumerable(layout, result);
  if (!capture.answered || !capture.valid || !capture.satisfied ||
      !supports(capture.enumerable.operations, sizeof(ttx_enumerable_ops))) {
    return false;
  }
  const ttx_layout candidate =
      capture.enumerable.operations->layout(capture.enumerable);
  if (candidate.owner != layout.owner || candidate.value != layout.value) {
    return false;
  }
  enumerable = capture.enumerable;
  return true;
}

static auto select(ttx_layout_snapshot_result self) -> SnapshotCapture& {
  static_assert(offsetof(SnapshotCapture, operations) == 0);
  return *reinterpret_cast<SnapshotCapture*>(
      const_cast<ttx_layout_snapshot_result_ops*>(self.operations));
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
    .owner = layout.owner,
    .value = layout.value,
  };
  layout.operations->snapshot(layout, result);
  return capture;
}

static auto select(ttx_composite_layout_result self) -> CompositeCapture& {
  static_assert(offsetof(CompositeCapture, operations) == 0);
  return *reinterpret_cast<CompositeCapture*>(
      const_cast<ttx_composite_layout_result_ops*>(self.operations));
}

static void TTX_CALL composite_rejected(ttx_composite_layout_result self) {
  CompositeCapture& capture = select(self);
  capture.answered = true;
  capture.satisfied = false;
}

static void TTX_CALL composite_satisfied(
    ttx_composite_layout_result self,
    ttx_composite_layout composite) {
  CompositeCapture& capture = select(self);
  capture.answered = true;
  capture.satisfied = true;
  capture.composite = composite;
}

static auto query_composite(ttx_layout layout, ttx_composite_layout& composite)
    -> bool {
  if (!supports(layout.operations, sizeof(ttx_layout_ops))) {
    return false;
  }
  CompositeCapture capture = {
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
  };
  const ttx_composite_layout_result result = {
    .operations = &capture.operations,
    .owner = layout.owner,
    .value = layout.value,
  };
  layout.operations->composite(layout, result);
  if (!capture.answered || !capture.satisfied ||
      !supports(
          capture.composite.operations, sizeof(ttx_composite_layout_ops))) {
    return false;
  }
  const ttx_layout candidate =
      capture.composite.operations->candidate(capture.composite);
  if (candidate.owner != layout.owner || candidate.value != layout.value) {
    return false;
  }
  composite = capture.composite;
  return true;
}

static auto select(ttx_layout_entry_sink self) -> VisitFrame& {
  static_assert(offsetof(VisitFrame, operations) == 0);
  return *reinterpret_cast<VisitFrame*>(
      const_cast<ttx_layout_entry_sink_ops*>(self.operations));
}

static void TTX_CALL visit_entry(
    ttx_layout_entry_sink self,
    ttx_borrowed_bytes path,
    ttx_abstract producer) {
  VisitFrame& frame = select(self);
  if (frame.completed || !frame.valid ||
      (path.size != 0 && path.data == nullptr)) {
    frame.valid = false;
    return;
  }
  std::vector<uint8_t> nested(path.size + 1);
  nested[0] = frame.prefix;
  for (uint64_t index = 0; index < path.size; ++index) {
    nested[index + 1] = path.data[index];
  }
  frame.result.operations->entry(
      frame.result,
      {
        .data = nested.data(),
        .size = nested.size(),
      },
      producer);
}

static void TTX_CALL visit_completed(ttx_layout_entry_sink self) {
  VisitFrame& frame = select(self);
  if (frame.completed) {
    frame.valid = false;
    return;
  }
  frame.completed = true;
}

static auto visit_child(
    ttx_enumerable enumerable,
    uint8_t prefix,
    ttx_layout_entry_sink result) -> bool {
  VisitFrame frame = {
    .operations =
        {
          .header =
              {
                .size = sizeof(ttx_layout_entry_sink_ops),
                .abi_major = TTX_ABI_MAJOR,
                .abi_minor = TTX_ABI_MINOR,
              },
          .entry = visit_entry,
          .completed = visit_completed,
        },
    .result = result,
    .prefix = prefix,
    .completed = false,
    .valid = true,
  };
  const ttx_layout_entry_sink sink = {
    .operations = &frame.operations,
    .owner = result.owner,
    .value = result.value,
  };
  enumerable.operations->visit(enumerable, sink);
  return frame.valid && frame.completed;
}

static auto TTX_CALL borrowed_layout(ttx_pack self) -> ttx_layout {
  if (self.operations == nullptr) {
    return {};
  }
  static_assert(offsetof(BorrowedPack, operations) == 0);
  return reinterpret_cast<const BorrowedPack*>(self.operations)->layout;
}

static auto borrowed_pack(ttx_layout layout) -> BorrowedPack {
  return {
    .operations =
        {
          .header =
              {
                .size = sizeof(ttx_pack_ops),
                .abi_major = TTX_ABI_MAJOR,
                .abi_minor = TTX_ABI_MINOR,
              },
          .layout = borrowed_layout,
        },
    .layout = layout,
  };
}

Composite::Composite(ttx_layout left, ttx_layout right)
    : Composite(left, right, {}, {}, ttx_authority_create(), 1) {}

Composite::Composite(
    ttx_layout left,
    ttx_layout right,
    ttx_layout_snapshot left_snapshot,
    ttx_layout_snapshot right_snapshot,
    uint64_t authority,
    uint64_t value)
    : Layout(authority, value),
      enumerable_binding({
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
      composite_binding({
        .operations =
            {
              .header =
                  {
                    .size = sizeof(ttx_composite_layout_ops),
                    .abi_major = TTX_ABI_MAJOR,
                    .abi_minor = TTX_ABI_MINOR,
                  },
              .candidate = composite_candidate,
              .left = composite_left,
              .right = composite_right,
            },
        .owner = this,
      }),
      left_layout(left),
      right_layout(right),
      left_snapshot(left_snapshot),
      right_snapshot(right_snapshot) {
  if (!supports(left.operations, sizeof(ttx_layout_ops)) ||
      !supports(right.operations, sizeof(ttx_layout_ops))) {
    std::abort();
  }
}

Composite::~Composite() {
  release(right_snapshot);
  release(left_snapshot);
}

void Composite::fit(
    ttx_pack source,
    ttx_context context,
    ttx_pack_result result) const {
  if (!supports(source.operations, sizeof(ttx_pack_ops)) ||
      !supports(context.operations, sizeof(ttx_context_ops))) {
    result.operations->support_failed(result, TTX_PACK_SUPPORT_INVALID_LAYOUT);
    return;
  }
  const ttx_layout source_layout = source.operations->layout(source);
  ttx_composite_layout source_composite;
  if (!query_composite(source_layout, source_composite)) {
    result.operations->none(result);
    return;
  }
  const ttx_layout source_left =
      source_composite.operations->left(source_composite);
  const ttx_layout source_right =
      source_composite.operations->right(source_composite);
  const BorrowedPack left_pack = borrowed_pack(source_left);
  const BorrowedPack right_pack = borrowed_pack(source_right);
  const ttx_context staging = ttx_context_create();
  if (!supports(staging.operations, sizeof(ttx_context_ops))) {
    result.operations->support_failed(result, TTX_PACK_SUPPORT_EXHAUSTED);
    return;
  }
  const ttx_pack left_source = {
    .operations = &left_pack.operations,
    .owner = source.owner,
    .value = source.value,
  };
  const ttx_pack right_source = {
    .operations = &right_pack.operations,
    .owner = source.owner,
    .value = source.value,
  };
  const PackObservation left = Ttx::fit(left_layout, left_source, staging);
  const PackObservation right = Ttx::fit(right_layout, right_source, staging);
  if (left.state == PackObservationState::SupportFailed ||
      right.state == PackObservationState::SupportFailed) {
    const ttx_pack_support_failure failure =
        left.state == PackObservationState::SupportFailed ? left.failure
                                                          : right.failure;
    staging.operations->release(staging);
    result.operations->support_failed(result, failure);
    return;
  }
  if (left.state == PackObservationState::None ||
      right.state == PackObservationState::None) {
    staging.operations->release(staging);
    result.operations->none(result);
    return;
  }
  if (left.state == PackObservationState::Unknown ||
      right.state == PackObservationState::Unknown) {
    staging.operations->release(staging);
    result.operations->unknown(result);
    return;
  }

  const ttx_layout fitted_left = left.pack.operations->layout(left.pack);
  const ttx_layout fitted_right = right.pack.operations->layout(right.pack);
  Composite witness(fitted_left, fitted_right);
  context.operations->pack(context, witness.get_abi(), result);
  staging.operations->release(staging);
}

void Composite::enumerable(ttx_enumerable_result result) const {
  ttx_enumerable left;
  ttx_enumerable right;
  if (!query_enumerable(left_layout, left) ||
      !query_enumerable(right_layout, right) ||
      left.operations->cardinality(left) >
          std::numeric_limits<uint64_t>::max() -
              right.operations->cardinality(right)) {
    result.operations->rejected(result);
    return;
  }
  const ttx_layout identity = get_abi();
  result.operations->satisfied(
      result, {
                .operations = &enumerable_binding.operations,
                .owner = identity.owner,
                .value = identity.value,
              });
}

void Composite::snapshot(ttx_layout_snapshot_result result) const {
  const SnapshotCapture left = capture_snapshot(left_layout);
  if (!left.answered || !left.valid || !left.retained ||
      !supports(left.snapshot.operations, sizeof(ttx_layout_snapshot_ops))) {
    result.operations->support_failed(result, left.failure);
    return;
  }
  const SnapshotCapture right = capture_snapshot(right_layout);
  if (!right.answered || !right.valid || !right.retained ||
      !supports(right.snapshot.operations, sizeof(ttx_layout_snapshot_ops))) {
    release(left.snapshot);
    result.operations->support_failed(result, right.failure);
    return;
  }
  const ttx_layout copied_left =
      left.snapshot.operations->layout(left.snapshot);
  const ttx_layout copied_right =
      right.snapshot.operations->layout(right.snapshot);
  const ttx_layout identity = get_abi();
  auto* copy = new (std::nothrow) Composite(
      copied_left, copied_right, left.snapshot, right.snapshot, identity.owner,
      identity.value);
  if (copy == nullptr) {
    release(right.snapshot);
    release(left.snapshot);
    result.operations->support_failed(result, TTX_PACK_SUPPORT_EXHAUSTED);
    return;
  }
  result.operations->retained(
      result, {
                .operations = &copy->snapshot_binding.operations,
                .owner = identity.owner,
                .value = identity.value,
              });
}

void Composite::composite(ttx_composite_layout_result result) const {
  const ttx_layout identity = get_abi();
  result.operations->satisfied(
      result, {
                .operations = &composite_binding.operations,
                .owner = identity.owner,
                .value = identity.value,
              });
}

template <typename Binding, typename Handle>
static auto select_owner(Handle self) -> Composite& {
  if (self.operations == nullptr) {
    std::abort();
  }
  static_assert(offsetof(Binding, operations) == 0);
  const auto& binding = *reinterpret_cast<const Binding*>(self.operations);
  if (binding.owner == nullptr) {
    std::abort();
  }
  const ttx_layout identity = binding.owner->get_abi();
  if (identity.owner != self.owner || identity.value != self.value) {
    std::abort();
  }
  return *const_cast<Composite*>(binding.owner);
}

auto Composite::select(ttx_enumerable self) -> const Composite& {
  return select_owner<EnumerableBinding>(self);
}

auto Composite::select(ttx_layout_snapshot self) -> Composite& {
  return select_owner<SnapshotBinding>(self);
}

auto Composite::select(ttx_composite_layout self) -> const Composite& {
  return select_owner<CompositeBinding>(self);
}

auto TTX_CALL Composite::enumerable_layout(ttx_enumerable self) -> ttx_layout {
  return select(self).get_abi();
}

auto TTX_CALL Composite::enumerable_cardinality(ttx_enumerable self)
    -> uint64_t {
  const Composite& selected = select(self);
  ttx_enumerable left;
  ttx_enumerable right;
  if (!query_enumerable(selected.left_layout, left) ||
      !query_enumerable(selected.right_layout, right)) {
    std::abort();
  }
  return left.operations->cardinality(left) +
         right.operations->cardinality(right);
}

void TTX_CALL Composite::enumerable_visit(
    ttx_enumerable self,
    ttx_layout_entry_sink result) {
  const Composite& selected = select(self);
  ttx_enumerable left;
  ttx_enumerable right;
  if (!query_enumerable(selected.left_layout, left) ||
      !query_enumerable(selected.right_layout, right) ||
      !visit_child(left, 0, result) || !visit_child(right, 1, result)) {
    result.operations->completed(result);
    return;
  }
  result.operations->completed(result);
}

auto TTX_CALL Composite::snapshot_layout(ttx_layout_snapshot self)
    -> ttx_layout {
  return select(self).get_abi();
}

void TTX_CALL Composite::snapshot_release(ttx_layout_snapshot self) {
  delete &select(self);
}

auto TTX_CALL Composite::composite_candidate(ttx_composite_layout self)
    -> ttx_layout {
  return select(self).get_abi();
}

auto TTX_CALL Composite::composite_left(ttx_composite_layout self)
    -> ttx_layout {
  return select(self).left_layout;
}

auto TTX_CALL Composite::composite_right(ttx_composite_layout self)
    -> ttx_layout {
  return select(self).right_layout;
}
