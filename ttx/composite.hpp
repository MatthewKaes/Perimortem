// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "ttx/layout.hpp"

namespace Ttx::Layouts {

// Composite preserves two ordered child Layouts as separate fitting
// boundaries. Enumeration may expose their leaves, but neither fitting nor a
// snapshot may flatten the children into one positional sequence.
class Composite final : public Layout {
 public:
  Composite(ttx_layout left, ttx_layout right);
  ~Composite() override;

  void fit(ttx_pack source, ttx_context context, ttx_pack_result result)
      const override;
  void enumerable(ttx_enumerable_result result) const override;
  void snapshot(ttx_layout_snapshot_result result) const override;
  void composite(ttx_composite_layout_result result) const override;

 private:
  struct EnumerableBinding {
    ttx_enumerable_ops operations;
    const Composite* owner;
  };

  struct SnapshotBinding {
    ttx_layout_snapshot_ops operations;
    Composite* owner;
  };

  struct CompositeBinding {
    ttx_composite_layout_ops operations;
    const Composite* owner;
  };

  Composite(
      ttx_layout left,
      ttx_layout right,
      ttx_layout_snapshot left_snapshot,
      ttx_layout_snapshot right_snapshot,
      uint64_t authority,
      uint64_t value);

  static auto select(ttx_enumerable self) -> const Composite&;
  static auto select(ttx_layout_snapshot self) -> Composite&;
  static auto select(ttx_composite_layout self) -> const Composite&;
  static auto TTX_CALL enumerable_layout(ttx_enumerable self) -> ttx_layout;
  static auto TTX_CALL enumerable_cardinality(ttx_enumerable self) -> uint64_t;
  static void TTX_CALL
      enumerable_visit(ttx_enumerable self, ttx_layout_entry_sink result);
  static auto TTX_CALL snapshot_layout(ttx_layout_snapshot self) -> ttx_layout;
  static void TTX_CALL snapshot_release(ttx_layout_snapshot self);
  static auto TTX_CALL composite_candidate(ttx_composite_layout self)
      -> ttx_layout;
  static auto TTX_CALL composite_left(ttx_composite_layout self) -> ttx_layout;
  static auto TTX_CALL composite_right(ttx_composite_layout self) -> ttx_layout;

  const EnumerableBinding enumerable_binding;
  const SnapshotBinding snapshot_binding;
  const CompositeBinding composite_binding;
  const ttx_layout left_layout;
  const ttx_layout right_layout;
  const ttx_layout_snapshot left_snapshot;
  const ttx_layout_snapshot right_snapshot;
};

}  // namespace Ttx::Layouts
