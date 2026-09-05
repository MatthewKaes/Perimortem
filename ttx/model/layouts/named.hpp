// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "ttx/concept/layout.hpp"

namespace Ttx::Layouts {

// Named layers one route Layout over one source Layout with the same exact
// enumerable paths. Route producers remain ordinary Abstracts and resolve to
// Unknown, None, or a complete Route only when a caller visits them.
class Named final : public Layout {
 public:
  Named(ttx_layout source, ttx_layout routes);
  ~Named() override;

  void fit(ttx_pack source, ttx_context context, ttx_pack_result result)
      const override;
  void enumerable(ttx_enumerable_result result) const override;
  void named(ttx_named_result result) const override;
  void snapshot(ttx_layout_snapshot_result result) const override;

 private:
  struct NamedBinding {
    ttx_named_ops operations;
  };

  struct SnapshotBinding {
    ttx_layout_snapshot_ops operations;
  };

  Named(
      ttx_layout source,
      ttx_layout routes,
      ttx_layout_snapshot source_snapshot,
      ttx_layout_snapshot routes_snapshot);

  auto valid() const -> bool;
  static auto select(ttx_named self) -> const Named&;
  static auto select(ttx_layout_snapshot self) -> Named&;
  static auto TTX_CALL candidate(ttx_named self) -> ttx_layout;
  static auto TTX_CALL source(ttx_named self) -> ttx_layout;
  static auto TTX_CALL routes(ttx_named self) -> ttx_layout;
  static void TTX_CALL
      visit_routes(ttx_named self, ttx_named_route_sink result);
  static void TTX_CALL select_route(
      ttx_named self,
      ttx_borrowed_bytes route,
      ttx_named_selection_result result);
  static auto TTX_CALL snapshot_layout(ttx_layout_snapshot self) -> ttx_layout;
  static void TTX_CALL snapshot_release(ttx_layout_snapshot self);

  const NamedBinding named_binding;
  const SnapshotBinding snapshot_binding;
  const ttx_layout source_layout;
  const ttx_layout routes_layout;
  const ttx_layout_snapshot source_snapshot;
  const ttx_layout_snapshot routes_snapshot;
};

}  // namespace Ttx::Layouts
