// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include <cstdint>
#include <vector>

#include "ttx/concept/layout.hpp"

namespace Ttx::Layouts {

// Reindexed lets an operation publish a new traversal without copying the
// values it selected. Projection describes the output shape with the original
// producer identities, while each mapping records which source occurrence was
// used for one output occurrence. Keeping both Layouts allows a consumer to
// verify that relationship without learning how names or indexes were resolved.
class Reindexed final : public Layout {
 public:
  struct Mapping {
    std::vector<uint8_t> output;
    std::vector<uint8_t> source;
  };

  Reindexed(
      ttx_layout source,
      ttx_layout projection,
      std::vector<Mapping> mappings);
  ~Reindexed() override;

  void fit(ttx_pack source, ttx_context context, ttx_pack_result result)
      const override;
  void enumerable(ttx_enumerable_result result) const override;
  void snapshot(ttx_layout_snapshot_result result) const override;
  void reindexed(ttx_reindexed_layout_result result) const override;

 private:
  struct EnumerableBinding {
    ttx_enumerable_ops operations;
  };

  struct SnapshotBinding {
    ttx_layout_snapshot_ops operations;
  };

  struct ReindexedBinding {
    ttx_reindexed_layout_ops operations;
  };

  Reindexed(
      ttx_layout source,
      ttx_layout projection,
      std::vector<Mapping> mappings,
      ttx_layout_snapshot source_snapshot,
      ttx_layout_snapshot projection_snapshot);

  auto valid() const -> bool;
  static auto select(ttx_enumerable self) -> const Reindexed&;
  static auto select(ttx_layout_snapshot self) -> Reindexed&;
  static auto select(ttx_reindexed_layout self) -> const Reindexed&;
  static auto TTX_CALL enumerable_layout(ttx_enumerable self) -> ttx_layout;
  static auto TTX_CALL enumerable_cardinality(ttx_enumerable self) -> uint64_t;
  static void TTX_CALL
      enumerable_visit(ttx_enumerable self, ttx_layout_entry_sink result);
  static auto TTX_CALL snapshot_layout(ttx_layout_snapshot self) -> ttx_layout;
  static void TTX_CALL snapshot_release(ttx_layout_snapshot self);
  static auto TTX_CALL reindexed_candidate(ttx_reindexed_layout self)
      -> ttx_layout;
  static auto TTX_CALL reindexed_source(ttx_reindexed_layout self)
      -> ttx_layout;
  static auto TTX_CALL reindexed_projection(ttx_reindexed_layout self)
      -> ttx_layout;
  static void TTX_CALL
      visit_mappings(ttx_reindexed_layout self, ttx_reindex_sink result);

  const EnumerableBinding enumerable_binding;
  const SnapshotBinding snapshot_binding;
  const ReindexedBinding reindexed_binding;
  const ttx_layout source_layout;
  const ttx_layout projection_layout;
  const std::vector<Mapping> mappings;
  const ttx_layout_snapshot source_snapshot;
  const ttx_layout_snapshot projection_snapshot;
};

}  // namespace Ttx::Layouts
