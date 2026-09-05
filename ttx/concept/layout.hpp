// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "ttx/abi.h"

namespace Ttx {

// Layout is the modern C++ owner for one identity-free structural projection.
// Its handle points directly to this owner for the borrowed lifetime rather
// than assigning graph identity to query-time support data.
class Layout {
 public:
  constexpr Layout()
      : binding({
          .operations =
              {
                .header =
                    {
                      .size = sizeof(ttx_layout_ops),
                      .abi_major = TTX_ABI_MAJOR,
                      .abi_minor = TTX_ABI_MINOR,
                    },
                .fit = fit_abi,
                .enumerable = enumerable_abi,
                .named = named_abi,
                .snapshot = snapshot_abi,
                .fluid = fluid_abi,
                .value = value_abi,
                .composite = composite_abi,
                .ranged = ranged_abi,
                .reindexed = reindexed_abi,
              },
        }) {}
  virtual constexpr ~Layout() = default;

  // Layout carries no semantic identity, so a native owner may copy or move
  // its support value while constructing a larger projection. The new C++
  // object lends its own direct capability; crossing an ABI lifetime still
  // requires snapshot(), which transfers independently owned support to the
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
  struct AbiBinding {
    ttx_layout_ops operations;
  };

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

  AbiBinding binding;
};

}  // namespace Ttx
