// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include <cstdint>
#include <vector>

#include "ttx/concept/layout.hpp"

namespace Ttx::Layouts {

// Value handles one producer, but an operation may need to expose several
// unrelated producers without inventing an aggregate Domain for them. Fluid
// records that finite flow as independently addressable occurrences. It copies
// the structural paths while borrowing each original Abstract from the graph
// owner, so rearranging the flow does not create replacement identities.
class Fluid final : public Layout {
 public:
  struct Entry {
    std::vector<uint8_t> path;
    ttx_abstract producer;
  };

  explicit Fluid(std::vector<Entry> entries);

  void fit(ttx_pack source, ttx_context context, ttx_pack_result result)
      const override;
  void enumerable(ttx_enumerable_result result) const override;
  void snapshot(ttx_layout_snapshot_result result) const override;
  void fluid(ttx_fluid_result result) const override;

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

  static auto select(ttx_enumerable self) -> const Fluid&;
  static auto select(ttx_layout_snapshot self) -> Fluid&;
  static auto select(ttx_fluid self) -> const Fluid&;
  static auto TTX_CALL enumerable_layout(ttx_enumerable self) -> ttx_layout;
  static auto TTX_CALL enumerable_cardinality(ttx_enumerable self) -> uint64_t;
  static void TTX_CALL
      enumerable_visit(ttx_enumerable self, ttx_layout_entry_sink result);
  static auto TTX_CALL snapshot_layout(ttx_layout_snapshot self) -> ttx_layout;
  static void TTX_CALL snapshot_release(ttx_layout_snapshot self);
  static auto TTX_CALL fluid_layout(ttx_fluid self) -> ttx_layout;

  const EnumerableBinding enumerable_binding;
  const SnapshotBinding snapshot_binding;
  const FluidBinding fluid_binding;
  const std::vector<Entry> entries;
};

}  // namespace Ttx::Layouts
