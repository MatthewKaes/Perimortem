// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "ttx/layout.hpp"

namespace Ttx::Layouts {

// Ranged preserves one repeated producer relationship and one exact extent
// provider. It becomes enumerable only when that provider proves the narrow
// Extent contract, so an unsettled extent can never appear as zero flow.
class Ranged final : public Layout {
 public:
  Ranged(ttx_abstract producer, ttx_abstract extent);
  Ranged(
      ttx_abstract producer,
      ttx_abstract extent,
      uint64_t authority,
      uint64_t value);

  void fit(ttx_pack source, ttx_context context, ttx_pack_result result)
      const override;
  void enumerable(ttx_enumerable_result result) const override;
  void snapshot(ttx_layout_snapshot_result result) const override;
  void ranged(ttx_ranged_layout_result result) const override;

 private:
  struct EnumerableBinding {
    ttx_enumerable_ops operations;
    const Ranged* owner;
  };

  struct SnapshotBinding {
    ttx_layout_snapshot_ops operations;
    Ranged* owner;
  };

  struct RangedBinding {
    ttx_ranged_layout_ops operations;
    const Ranged* owner;
  };

  static auto select(ttx_enumerable self) -> const Ranged&;
  static auto select(ttx_layout_snapshot self) -> Ranged&;
  static auto select(ttx_ranged_layout self) -> const Ranged&;
  static auto TTX_CALL enumerable_layout(ttx_enumerable self) -> ttx_layout;
  static auto TTX_CALL enumerable_cardinality(ttx_enumerable self) -> uint64_t;
  static void TTX_CALL
      enumerable_visit(ttx_enumerable self, ttx_layout_entry_sink result);
  static auto TTX_CALL snapshot_layout(ttx_layout_snapshot self) -> ttx_layout;
  static void TTX_CALL snapshot_release(ttx_layout_snapshot self);
  static auto TTX_CALL ranged_candidate(ttx_ranged_layout self) -> ttx_layout;
  static auto TTX_CALL ranged_producer(ttx_ranged_layout self) -> ttx_abstract;
  static auto TTX_CALL ranged_extent(ttx_ranged_layout self) -> ttx_abstract;

  const EnumerableBinding enumerable_binding;
  const SnapshotBinding snapshot_binding;
  const RangedBinding ranged_binding;
  const ttx_abstract producer;
  const ttx_abstract extent;
};

}  // namespace Ttx::Layouts
