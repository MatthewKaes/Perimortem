// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <new>

#include "ttx/abi.h"
#include "ttx/query.hpp"

struct PackOwner {
  const ttx_pack_ops operations;
  PackOwner* previous;
  ttx_layout_snapshot snapshot;

  PackOwner(PackOwner* previous, ttx_layout_snapshot snapshot);
  ~PackOwner();
};

struct ContextOwner {
  // Context storage uses ordinary C++ allocation because its lifetime belongs
  // to the ABI caller rather than one Perimortem worker. Bibliotheca currently
  // pins reclamation to the allocating worker, which would add a hidden thread
  // obligation to an otherwise host neutral support contract.
  const ttx_context_ops operations;
  PackOwner* packs;

  ContextOwner();
  ~ContextOwner();
};

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

struct EntryValidation {
  ttx_layout_entry_sink_ops operations;
  uint64_t expected;
  uint64_t observed;
  bool completed;
  bool valid;
};

template <typename Operations>
static auto supports(const Operations* operations, uint32_t size) -> bool {
  return operations != nullptr &&
         operations->header.abi_major == TTX_ABI_MAJOR &&
         operations->header.size >= size;
}

static auto select_context(ttx_context self) -> ContextOwner& {
  if (self.self == nullptr) {
    std::abort();
  }
  ContextOwner& selected = *reinterpret_cast<ContextOwner*>(self.self);
  if (&selected.operations != self.operations) {
    std::abort();
  }
  return selected;
}

static auto select_pack(ttx_pack self) -> PackOwner& {
  if (self.self == nullptr) {
    std::abort();
  }
  PackOwner& selected = *reinterpret_cast<PackOwner*>(self.self);
  if (&selected.operations != self.operations) {
    std::abort();
  }
  return selected;
}

static auto select_snapshot(ttx_layout_snapshot_result self)
    -> SnapshotCapture& {
  return *reinterpret_cast<SnapshotCapture*>(self.self);
}

static auto select_enumerable(ttx_enumerable_result self)
    -> EnumerableCapture& {
  return *reinterpret_cast<EnumerableCapture*>(self.self);
}

static auto select_validation(ttx_layout_entry_sink self) -> EntryValidation& {
  return *reinterpret_cast<EntryValidation*>(self.self);
}

static void TTX_CALL snapshot_retained(
    ttx_layout_snapshot_result self,
    ttx_layout_snapshot snapshot) {
  SnapshotCapture& capture = select_snapshot(self);
  if (capture.answered) {
    capture.valid = false;
    if (snapshot.operations != nullptr &&
        snapshot.operations->header.abi_major == TTX_ABI_MAJOR &&
        snapshot.operations->header.size >= sizeof(ttx_layout_snapshot_ops)) {
      snapshot.operations->release(snapshot);
    }
    return;
  }
  capture.answered = true;
  capture.retained = true;
  capture.snapshot = snapshot;
}

static void TTX_CALL snapshot_failed(
    ttx_layout_snapshot_result self,
    ttx_pack_support_failure failure) {
  SnapshotCapture& capture = select_snapshot(self);
  if (capture.answered) {
    capture.valid = false;
    return;
  }
  capture.answered = true;
  capture.retained = false;
  capture.failure = failure;
}

static void TTX_CALL enumerable_rejected(ttx_enumerable_result self) {
  EnumerableCapture& capture = select_enumerable(self);
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
  EnumerableCapture& capture = select_enumerable(self);
  if (capture.answered) {
    capture.valid = false;
    return;
  }
  capture.answered = true;
  capture.satisfied = true;
  capture.enumerable = enumerable;
}

static void TTX_CALL validate_entry(
    ttx_layout_entry_sink self,
    ttx_borrowed_bytes path,
    ttx_abstract producer) {
  EntryValidation& validation = select_validation(self);
  if (validation.completed || validation.observed == validation.expected ||
      producer == nullptr || producer->operations == nullptr ||
      (path.size != 0 && path.data == nullptr)) {
    validation.valid = false;
    return;
  }

  // Unknown may occupy a position whose Layout owner independently proves the
  // occurrence. None cannot because it has already proved semantic absence.
  // An exact Domain with an enumerable empty Layout likewise carries context
  // but cannot become ordinary value flow.
  if (ttx_abstract_same(producer, ttx_none())) {
    validation.valid = false;
    return;
  }
  const Ttx::DomainObservation domain = Ttx::resolve_domain(producer);
  if (domain.state == Ttx::Observation::None) {
    validation.valid = false;
    return;
  }
  if (domain.state == Ttx::Observation::Resolved &&
      supports(domain.layout.operations, sizeof(ttx_layout_ops))) {
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
    domain.layout.operations->enumerable(domain.layout, result);
    if (capture.answered && capture.valid && capture.satisfied &&
        supports(capture.enumerable.operations, sizeof(ttx_enumerable_ops)) &&
        capture.enumerable.operations->cardinality(capture.enumerable) == 0) {
      validation.valid = false;
      return;
    }
  }
  ++validation.observed;
}

static void TTX_CALL validation_completed(ttx_layout_entry_sink self) {
  EntryValidation& validation = select_validation(self);
  if (validation.completed) {
    validation.valid = false;
    return;
  }
  validation.completed = true;
}

static auto validate_snapshot(ttx_layout_snapshot snapshot) -> bool {
  // Context accepts only a complete owned snapshot. Validating the transferred
  // operations first keeps malformed foreign support from becoming a Pack that
  // appears to have a trustworthy lifetime.
  if (!supports(snapshot.operations, sizeof(ttx_layout_snapshot_ops))) {
    return false;
  }
  const ttx_layout layout = snapshot.operations->layout(snapshot);
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

  EntryValidation validation = {
    .operations =
        {
          .header =
              {
                .size = sizeof(ttx_layout_entry_sink_ops),
                .abi_major = TTX_ABI_MAJOR,
                .abi_minor = TTX_ABI_MINOR,
              },
          .entry = validate_entry,
          .completed = validation_completed,
        },
    .expected = capture.enumerable.operations->cardinality(capture.enumerable),
    .observed = 0,
    .completed = false,
    .valid = true,
  };
  const ttx_layout_entry_sink entries = {
    .operations = &validation.operations,
    .self = reinterpret_cast<ttx_layout_entry_sink_self*>(&validation),
  };
  capture.enumerable.operations->visit(capture.enumerable, entries);

  // Exact cardinality keeps an empty flow distinct from a Layout whose
  // unsettled positions were never visited. Producer identities remain
  // borrowed from the graph and are never copied into Context policy.
  return validation.valid && validation.completed &&
         validation.observed == validation.expected;
}

static void release_snapshot(ttx_layout_snapshot snapshot) {
  if (supports(snapshot.operations, sizeof(ttx_layout_snapshot_ops))) {
    snapshot.operations->release(snapshot);
  }
}

static void TTX_CALL context_release(ttx_context self) {
  delete &select_context(self);
}

static void TTX_CALL context_pack(
    ttx_context self,
    ttx_layout produced_flow,
    ttx_pack_result result) {
  if (!supports(result.operations, sizeof(ttx_pack_result_ops)) ||
      !supports(produced_flow.operations, sizeof(ttx_layout_ops))) {
    return;
  }

  ContextOwner& context = select_context(self);
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
  const ttx_layout_snapshot_result snapshot_result = {
    .operations = &capture.operations,
    .self = reinterpret_cast<ttx_layout_snapshot_result_self*>(&capture),
  };
  produced_flow.operations->snapshot(produced_flow, snapshot_result);
  if (!capture.answered || !capture.valid || !capture.retained) {
    if (capture.retained) {
      release_snapshot(capture.snapshot);
    }
    result.operations->support_failed(result, capture.failure);
    return;
  }

  // A Layout owns the copy because only that owner can preserve its complete
  // structure and fitting behavior. Context validates the transferred snapshot
  // before publishing a Pack, then treats it as opaque support for its
  // lifetime.
  if (!validate_snapshot(capture.snapshot)) {
    release_snapshot(capture.snapshot);
    result.operations->support_failed(result, TTX_PACK_SUPPORT_INVALID_LAYOUT);
    return;
  }

  PackOwner* pack =
      new (std::nothrow) PackOwner(context.packs, capture.snapshot);
  if (pack == nullptr) {
    release_snapshot(capture.snapshot);
    result.operations->support_failed(result, TTX_PACK_SUPPORT_EXHAUSTED);
    return;
  }
  context.packs = pack;
  const ttx_pack retained = {
    .operations = &pack->operations,
    .self = reinterpret_cast<ttx_pack_self*>(pack),
  };
  result.operations->packed(result, retained);
}

static auto TTX_CALL pack_layout(ttx_pack self) -> ttx_layout {
  PackOwner& selected = select_pack(self);
  return selected.snapshot.operations->layout(selected.snapshot);
}

PackOwner::PackOwner(PackOwner* previous, ttx_layout_snapshot snapshot)
    : operations({
        .header =
            {
              .size = sizeof(ttx_pack_ops),
              .abi_major = TTX_ABI_MAJOR,
              .abi_minor = TTX_ABI_MINOR,
            },
        .layout = pack_layout,
      }),
      previous(previous),
      snapshot(snapshot) {}

PackOwner::~PackOwner() {
  release_snapshot(snapshot);
}

ContextOwner::ContextOwner()
    : operations({
        .header =
            {
              .size = sizeof(ttx_context_ops),
              .abi_major = TTX_ABI_MAJOR,
              .abi_minor = TTX_ABI_MINOR,
            },
        .release = context_release,
        .pack = context_pack,
      }),
      packs(nullptr) {}

ContextOwner::~ContextOwner() {
  while (packs != nullptr) {
    PackOwner* selected = packs;
    packs = selected->previous;
    delete selected;
  }
}

ttx_context TTX_CALL ttx_context_create(void) {
  ContextOwner* owner = new (std::nothrow) ContextOwner();
  if (owner == nullptr) {
    return {};
  }
  return {
    .operations = &owner->operations,
    .self = reinterpret_cast<ttx_context_self*>(owner),
  };
}
