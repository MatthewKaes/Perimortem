// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "ttx/abi.h"

namespace Ttx {

// Layout is the modern C++ owner for one identity free structural projection.
// Authority and value tokens support private dispatch only and never make the
// projection a semantic graph identity.
class Layout {
 public:
  Layout();
  Layout(uint64_t authority, uint64_t value);
  virtual ~Layout() = default;

  Layout(const Layout&) = delete;
  Layout(Layout&&) = delete;
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

 private:
  struct Binding {
    ttx_layout_ops operations;
    const Layout* owner;
    uint64_t authority;
    uint64_t value;
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

  const Binding binding;
};

}  // namespace Ttx
