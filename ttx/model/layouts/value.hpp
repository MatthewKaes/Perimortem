// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "ttx/concept/layout.hpp"

namespace Ttx::Layouts {

// Value is the smallest bridge from one producer into Pack flow. It retains the
// producer itself instead of copying its Domain into a tuple slot, which leaves
// every later consumer free to ask that producer a different question. Unknown
// may occupy the slot because Value, rather than the producer's current Domain
// answer, establishes that this one position exists. None cannot occupy it
// because None has already completed the question with absence.
class Value final : public Layout {
 public:
  explicit Value(ttx_abstract producer);

  void fit(ttx_pack source, ttx_context context, ttx_pack_result result)
      const override;
  void enumerable(ttx_enumerable_result result) const override;
  void snapshot(ttx_layout_snapshot_result result) const override;
  void fluid(ttx_fluid_result result) const override;
  void value(ttx_value_layout_result result) const override;

 private:
  struct EnumerableBinding {
    ttx_enumerable_ops operations;
  };

  struct SnapshotBinding {
    ttx_layout_snapshot_ops operations;
  };

  struct FluidBinding {
    ttx_fluid_ops operations;
  };

  struct ValueBinding {
    ttx_value_layout_ops operations;
  };

  static auto select(ttx_enumerable self) -> const Value&;
  static auto select(ttx_layout_snapshot self) -> Value&;
  static auto select(ttx_fluid self) -> const Value&;
  static auto select(ttx_value_layout self) -> const Value&;
  static auto TTX_CALL enumerable_layout(ttx_enumerable self) -> ttx_layout;
  static auto TTX_CALL enumerable_cardinality(ttx_enumerable self) -> uint64_t;
  static void TTX_CALL
      enumerable_visit(ttx_enumerable self, ttx_layout_entry_sink result);
  static auto TTX_CALL snapshot_layout(ttx_layout_snapshot self) -> ttx_layout;
  static void TTX_CALL snapshot_release(ttx_layout_snapshot self);
  static auto TTX_CALL fluid_layout(ttx_fluid self) -> ttx_layout;
  static auto TTX_CALL value_layout(ttx_value_layout self) -> ttx_layout;
  static auto TTX_CALL value_producer(ttx_value_layout self) -> ttx_abstract;

  const EnumerableBinding enumerable_binding;
  const SnapshotBinding snapshot_binding;
  const FluidBinding fluid_binding;
  const ValueBinding value_binding;
  const ttx_abstract producer;
};

}  // namespace Ttx::Layouts
