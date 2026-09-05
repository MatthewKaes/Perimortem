// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "ttx/model/layouts/fluid.hpp"

#include <cstddef>
#include <cstdlib>
#include <new>
#include <utility>

#include "ttx/concept/fitting.hpp"
#include "ttx/query.hpp"

using namespace Ttx;
using namespace Ttx::Layouts;

struct FluidCapture {
  ttx_fluid_result_ops operations;
  bool answered;
  bool satisfied;
  ttx_fluid fluid;
};

struct EnumerableCapture {
  ttx_enumerable_result_ops operations;
  bool answered;
  bool satisfied;
  ttx_enumerable enumerable;
};

struct EntryCapture {
  ttx_layout_entry_sink_ops operations;
  std::vector<Fluid::Entry>* entries;
  uint64_t expected;
  bool completed;
  bool valid;
};

static auto select(ttx_fluid_result self) -> FluidCapture& {
  return *reinterpret_cast<FluidCapture*>(self.self);
}

static void TTX_CALL fluid_rejected(ttx_fluid_result self) {
  FluidCapture& capture = select(self);
  capture.answered = true;
  capture.satisfied = false;
}

static void TTX_CALL fluid_satisfied(ttx_fluid_result self, ttx_fluid fluid) {
  FluidCapture& capture = select(self);
  capture.answered = true;
  capture.satisfied = true;
  capture.fluid = fluid;
}

static auto select(ttx_enumerable_result self) -> EnumerableCapture& {
  return *reinterpret_cast<EnumerableCapture*>(self.self);
}

static void TTX_CALL enumerable_rejected(ttx_enumerable_result self) {
  EnumerableCapture& capture = select(self);
  capture.answered = true;
  capture.satisfied = false;
}

static void TTX_CALL enumerable_satisfied(
    ttx_enumerable_result self,
    ttx_enumerable enumerable) {
  EnumerableCapture& capture = select(self);
  capture.answered = true;
  capture.satisfied = true;
  capture.enumerable = enumerable;
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
  std::vector<uint8_t> retained(path.size);
  for (uint64_t index = 0; index < path.size; ++index) {
    retained[index] = path.data[index];
  }
  capture.entries->push_back({
    .path = std::move(retained),
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

static auto enumerate(ttx_layout layout, std::vector<Fluid::Entry>& entries)
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
    .satisfied = false,
    .enumerable = {},
  };
  const ttx_enumerable_result result = {
    .operations = &capture.operations,
    .self = reinterpret_cast<ttx_enumerable_result_self*>(&capture),
  };
  layout.operations->enumerable(layout, result);
  if (!capture.answered || !capture.satisfied ||
      capture.enumerable.operations == nullptr) {
    return false;
  }
  const uint64_t cardinality =
      capture.enumerable.operations->cardinality(capture.enumerable);
  EntryCapture entry_capture = {
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
  const ttx_layout_entry_sink visitor = {
    .operations = &entry_capture.operations,
    .self = reinterpret_cast<ttx_layout_entry_sink_self*>(&entry_capture),
  };
  capture.enumerable.operations->visit(capture.enumerable, visitor);
  return entry_capture.valid && entry_capture.completed &&
         entries.size() == cardinality;
}

Fluid::Fluid(std::vector<Entry> entries)
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
      fluid_binding({
        .operations =
            {
              .header =
                  {
                    .size = sizeof(ttx_fluid_ops),
                    .abi_major = TTX_ABI_MAJOR,
                    .abi_minor = TTX_ABI_MINOR,
                  },
              .layout = fluid_layout,
            },
      }),
      entries(std::move(entries)) {
  for (const Entry& entry : this->entries) {
    if (entry.producer == nullptr || entry.producer->operations == nullptr) {
      std::abort();
    }
  }
}

void Fluid::fit(ttx_pack source, ttx_context context, ttx_pack_result result)
    const {
  if (source.operations == nullptr || context.operations == nullptr) {
    result.operations->support_failed(result, TTX_PACK_SUPPORT_INVALID_LAYOUT);
    return;
  }
  const ttx_layout source_layout = source.operations->layout(source);
  if (source_layout.operations == nullptr ||
      source_layout.operations->header.abi_major != TTX_ABI_MAJOR ||
      source_layout.operations->header.size < sizeof(ttx_layout_ops)) {
    result.operations->support_failed(result, TTX_PACK_SUPPORT_INVALID_LAYOUT);
    return;
  }

  std::vector<Entry> source_entries;
  if (entries.empty()) {
    if (!enumerate(source_layout, source_entries)) {
      result.operations->support_failed(
          result, TTX_PACK_SUPPORT_INVALID_LAYOUT);
    } else if (source_entries.empty()) {
      context.operations->pack(context, source_layout, result);
    } else {
      result.operations->none(result);
    }
    return;
  }

  // Enumerable alone would flatten a Composite or another stronger Layout.
  // Requiring Fluid first proves that these leaves are independently
  // positional before this receiving owner aligns them by order.
  FluidCapture fluid = {
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
    .fluid = {},
  };
  const ttx_fluid_result fluid_result = {
    .operations = &fluid.operations,
    .self = reinterpret_cast<ttx_fluid_result_self*>(&fluid),
  };
  source_layout.operations->fluid(source_layout, fluid_result);
  if (!fluid.answered || !fluid.satisfied ||
      fluid.fluid.operations == nullptr) {
    result.operations->none(result);
    return;
  }
  const ttx_layout positional = fluid.fluid.operations->layout(fluid.fluid);
  if (!enumerate(positional, source_entries)) {
    result.operations->support_failed(result, TTX_PACK_SUPPORT_INVALID_LAYOUT);
    return;
  }
  if (source_entries.size() != entries.size()) {
    result.operations->none(result);
    return;
  }

  std::vector<Entry> admitted;
  admitted.reserve(entries.size());
  for (uint64_t index = 0; index < entries.size(); ++index) {
    const ttx_abstract producer = source_entries[index].producer;
    const LeafFit fit = Ttx::fit_leaf(producer, entries[index].producer);
    if (fit == LeafFit::Accepted) {
      admitted.push_back({.path = entries[index].path, .producer = producer});
      continue;
    }
    if (fit == LeafFit::Unknown) {
      admitted.push_back(
          {.path = entries[index].path, .producer = ttx_unknown()});
      continue;
    }
    result.operations->none(result);
    return;
  }

  // The witness preserves the receiver's shape while carrying the exact source
  // producers admitted at each position. Context owns its copied snapshot, so
  // neither participating Pack acquires the directional relationship.
  Fluid witness(std::move(admitted));
  context.operations->pack(context, witness.get_abi(), result);
}

void Fluid::enumerable(ttx_enumerable_result result) const {
  const ttx_enumerable view = {
    .operations = &enumerable_binding.operations,
    .self = reinterpret_cast<ttx_enumerable_self*>(const_cast<Fluid*>(this)),
  };
  result.operations->satisfied(result, view);
}

void Fluid::snapshot(ttx_layout_snapshot_result result) const {
  auto* copy = new (std::nothrow) Fluid(entries);
  if (copy == nullptr) {
    result.operations->support_failed(result, TTX_PACK_SUPPORT_EXHAUSTED);
    return;
  }
  const ttx_layout_snapshot snapshot = {
    .operations = &copy->snapshot_binding.operations,
    .self = reinterpret_cast<ttx_layout_snapshot_self*>(copy),
  };
  result.operations->retained(result, snapshot);
}

void Fluid::fluid(ttx_fluid_result result) const {
  const ttx_fluid view = {
    .operations = &fluid_binding.operations,
    .self = reinterpret_cast<ttx_fluid_self*>(const_cast<Fluid*>(this)),
  };
  result.operations->satisfied(result, view);
}

auto Fluid::select(ttx_enumerable self) -> const Fluid& {
  if (self.operations == nullptr || self.self == nullptr) {
    std::abort();
  }
  return *reinterpret_cast<const Fluid*>(self.self);
}

auto Fluid::select(ttx_layout_snapshot self) -> Fluid& {
  if (self.operations == nullptr || self.self == nullptr) {
    std::abort();
  }
  return *reinterpret_cast<Fluid*>(self.self);
}

auto Fluid::select(ttx_fluid self) -> const Fluid& {
  if (self.operations == nullptr || self.self == nullptr) {
    std::abort();
  }
  return *reinterpret_cast<const Fluid*>(self.self);
}

auto TTX_CALL Fluid::enumerable_layout(ttx_enumerable self) -> ttx_layout {
  return select(self).get_abi();
}

auto TTX_CALL Fluid::enumerable_cardinality(ttx_enumerable self) -> uint64_t {
  return select(self).entries.size();
}

void TTX_CALL
    Fluid::enumerable_visit(ttx_enumerable self, ttx_layout_entry_sink result) {
  for (const Entry& entry : select(self).entries) {
    result.operations->entry(
        result,
        {
          .data = entry.path.data(),
          .size = entry.path.size(),
        },
        entry.producer);
  }
  result.operations->completed(result);
}

auto TTX_CALL Fluid::snapshot_layout(ttx_layout_snapshot self) -> ttx_layout {
  return select(self).get_abi();
}

void TTX_CALL Fluid::snapshot_release(ttx_layout_snapshot self) {
  delete &select(self);
}

auto TTX_CALL Fluid::fluid_layout(ttx_fluid self) -> ttx_layout {
  return select(self).get_abi();
}
