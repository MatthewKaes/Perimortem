// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "ttx/value_layout.hpp"

#include <cstddef>
#include <cstdlib>
#include <new>

#include "ttx/fitting.hpp"

using namespace Ttx;
using namespace Ttx::Layouts;

struct ValueCapture {
  ttx_value_layout_result_ops operations;
  bool answered;
  bool satisfied;
  ttx_value_layout value;
};

static auto select(ttx_value_layout_result self) -> ValueCapture& {
  static_assert(offsetof(ValueCapture, operations) == 0);
  return *reinterpret_cast<ValueCapture*>(
      const_cast<ttx_value_layout_result_ops*>(self.operations));
}

static void TTX_CALL value_rejected(ttx_value_layout_result self) {
  ValueCapture& capture = select(self);
  capture.answered = true;
  capture.satisfied = false;
}

static void TTX_CALL
    value_satisfied(ttx_value_layout_result self, ttx_value_layout value) {
  ValueCapture& capture = select(self);
  capture.answered = true;
  capture.satisfied = true;
  capture.value = value;
}

Value::Value(ttx_abstract producer)
    : Value(producer, ttx_authority_create(), 1) {}

Value::Value(ttx_abstract producer, uint64_t authority, uint64_t value)
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
        .owner = this,
      }),
      value_binding({
        .operations =
            {
              .header =
                  {
                    .size = sizeof(ttx_value_layout_ops),
                    .abi_major = TTX_ABI_MAJOR,
                    .abi_minor = TTX_ABI_MINOR,
                  },
              .layout = value_layout,
              .producer = value_producer,
            },
        .owner = this,
      }),
      producer(producer) {
  if (producer.operations == nullptr ||
      ttx_abstract_same(producer, ttx_none())) {
    std::abort();
  }
}

void Value::fit(ttx_pack source, ttx_context context, ttx_pack_result result)
    const {
  if (source.operations == nullptr || context.operations == nullptr) {
    result.operations->support_failed(result, TTX_PACK_SUPPORT_INVALID_LAYOUT);
    return;
  }
  const ttx_layout source_layout = source.operations->layout(source);
  if (source_layout.operations == nullptr) {
    result.operations->support_failed(result, TTX_PACK_SUPPORT_INVALID_LAYOUT);
    return;
  }
  ValueCapture capture = {
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
  };
  const ttx_value_layout_result view = {
    .operations = &capture.operations,
    .owner = source_layout.owner,
    .value = source_layout.value,
  };
  source_layout.operations->value(source_layout, view);
  if (!capture.answered || !capture.satisfied ||
      capture.value.operations == nullptr) {
    result.operations->none(result);
    return;
  }
  const ttx_abstract admitted =
      capture.value.operations->producer(capture.value);
  const LeafFit fit = Ttx::fit_leaf(admitted, producer);
  if (fit == LeafFit::None) {
    result.operations->none(result);
    return;
  }

  // The fitted witness keeps the receiving Value shape but maps its one leaf
  // back to the exact source producer admitted by this observation.
  Value witness(fit == LeafFit::Unknown ? ttx_unknown() : admitted);
  context.operations->pack(context, witness.get_abi(), result);
}

void Value::enumerable(ttx_enumerable_result result) const {
  const ttx_layout identity = get_abi();
  result.operations->satisfied(
      result, {
                .operations = &enumerable_binding.operations,
                .owner = identity.owner,
                .value = identity.value,
              });
}

void Value::snapshot(ttx_layout_snapshot_result result) const {
  const ttx_layout identity = get_abi();
  auto* copy =
      new (std::nothrow) Value(producer, identity.owner, identity.value);
  if (copy == nullptr) {
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

void Value::fluid(ttx_fluid_result result) const {
  const ttx_layout identity = get_abi();
  result.operations->satisfied(
      result, {
                .operations = &fluid_binding.operations,
                .owner = identity.owner,
                .value = identity.value,
              });
}

void Value::value(ttx_value_layout_result result) const {
  const ttx_layout identity = get_abi();
  result.operations->satisfied(
      result, {
                .operations = &value_binding.operations,
                .owner = identity.owner,
                .value = identity.value,
              });
}

template <typename Binding, typename Handle>
static auto select_owner(Handle self) -> Value& {
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
  return *const_cast<Value*>(binding.owner);
}

auto Value::select(ttx_enumerable self) -> const Value& {
  return select_owner<EnumerableBinding>(self);
}

auto Value::select(ttx_layout_snapshot self) -> Value& {
  return select_owner<SnapshotBinding>(self);
}

auto Value::select(ttx_fluid self) -> const Value& {
  return select_owner<FluidBinding>(self);
}

auto Value::select(ttx_value_layout self) -> const Value& {
  return select_owner<ValueBinding>(self);
}

auto TTX_CALL Value::enumerable_layout(ttx_enumerable self) -> ttx_layout {
  return select(self).get_abi();
}

auto TTX_CALL Value::enumerable_cardinality(ttx_enumerable self) -> uint64_t {
  (void)self;
  return 1;
}

void TTX_CALL
    Value::enumerable_visit(ttx_enumerable self, ttx_layout_entry_sink result) {
  result.operations->entry(result, {}, select(self).producer);
  result.operations->completed(result);
}

auto TTX_CALL Value::snapshot_layout(ttx_layout_snapshot self) -> ttx_layout {
  return select(self).get_abi();
}

void TTX_CALL Value::snapshot_release(ttx_layout_snapshot self) {
  delete &select(self);
}

auto TTX_CALL Value::fluid_layout(ttx_fluid self) -> ttx_layout {
  return select(self).get_abi();
}

auto TTX_CALL Value::value_layout(ttx_value_layout self) -> ttx_layout {
  return select(self).get_abi();
}

auto TTX_CALL Value::value_producer(ttx_value_layout self) -> ttx_abstract {
  return select(self).producer;
}
