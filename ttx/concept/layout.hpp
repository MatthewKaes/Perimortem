// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "ttx/abi.h"

namespace Ttx {

// An operation can assemble a shape for just one query. These overrides let
// that shape offer fitting and structural views through the same C ABI used
// by foreign producers. Sharing the dispatch table leaves each temporary with
// only its own state. Its caller borrows the projection during that operation
// or asks snapshot() to preserve the support it needs afterward.
class Layout {
 public:
  constexpr Layout() = default;
  virtual constexpr ~Layout() = default;

  // Layout carries no semantic identity, so a native owner may copy or move
  // its support value while constructing a larger projection. The new C++
  // object lends its own direct capability. Crossing an ABI lifetime requires
  // snapshot(), which transfers independently owned support to the
  // caller rather than borrowing this native copy.
  constexpr Layout(const Layout&) : Layout() {}
  constexpr Layout(Layout&&) : Layout() {}
  auto operator=(const Layout&) -> Layout& = delete;
  auto operator=(Layout&&) -> Layout& = delete;

  auto get_abi() const -> ttx_layout;

  virtual void fit(ttx_pack source, ttx_context context, ttx_pack_result result)
      const;
  virtual void enumerable(ttx_enumerable_result result) const = 0;
  virtual void named(ttx_named_result result) const;
  virtual void snapshot(ttx_layout_snapshot_result result) const = 0;
  virtual void fluid(ttx_fluid_result result) const;
  virtual void value(ttx_value_layout_result result) const;
  virtual void composite(ttx_composite_layout_result result) const;
  virtual void ranged(ttx_ranged_layout_result result) const;
  virtual void reindexed(ttx_reindexed_layout_result result) const;

 private:
  static const ttx_layout_ops operations;

  static auto select(ttx_layout self) -> const Layout&;
  static void TTX_CALL fit_abi(
      ttx_layout self,
      ttx_pack source,
      ttx_context context,
      ttx_pack_result result);
  static void TTX_CALL
      enumerable_abi(ttx_layout self, ttx_enumerable_result result);
  static void TTX_CALL named_abi(ttx_layout self, ttx_named_result result);
  static void TTX_CALL
      snapshot_abi(ttx_layout self, ttx_layout_snapshot_result result);
  static void TTX_CALL fluid_abi(ttx_layout self, ttx_fluid_result result);
  static void TTX_CALL
      value_abi(ttx_layout self, ttx_value_layout_result result);
  static void TTX_CALL
      composite_abi(ttx_layout self, ttx_composite_layout_result result);
  static void TTX_CALL
      ranged_abi(ttx_layout self, ttx_ranged_layout_result result);
  static void TTX_CALL
      reindexed_abi(ttx_layout self, ttx_reindexed_layout_result result);
};

}  // namespace Ttx
