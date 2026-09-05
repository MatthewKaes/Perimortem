// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "ttx/concept/layout.hpp"

#include <cstddef>
#include <cstdlib>

using namespace Ttx;

auto Layout::get_abi() const -> ttx_layout {
  return {
    .operations = &binding.operations,
    .self = reinterpret_cast<ttx_layout_self*>(const_cast<Layout*>(this)),
  };
}

void Layout::fit(ttx_pack, ttx_context, ttx_pack_result result) const {
  result.operations->none(result);
}

void Layout::named(ttx_named_result result) const {
  result.operations->rejected(result);
}

void Layout::fluid(ttx_fluid_result result) const {
  result.operations->rejected(result);
}

void Layout::value(ttx_value_layout_result result) const {
  result.operations->rejected(result);
}

void Layout::composite(ttx_composite_layout_result result) const {
  result.operations->rejected(result);
}

void Layout::ranged(ttx_ranged_layout_result result) const {
  result.operations->rejected(result);
}

void Layout::reindexed(ttx_reindexed_layout_result result) const {
  result.operations->rejected(result);
}

auto Layout::select(ttx_layout self) -> const Layout& {
  if (self.operations == nullptr || self.self == nullptr) {
    std::abort();
  }
  const auto& selected = *reinterpret_cast<const Layout*>(self.self);
  if (&selected.binding.operations != self.operations) {
    std::abort();
  }
  return selected;
}

void TTX_CALL Layout::fit_abi(
    ttx_layout self,
    ttx_pack source,
    ttx_context context,
    ttx_pack_result result) {
  select(self).fit(source, context, result);
}

void TTX_CALL
    Layout::enumerable_abi(ttx_layout self, ttx_enumerable_result result) {
  select(self).enumerable(result);
}

void TTX_CALL Layout::named_abi(ttx_layout self, ttx_named_result result) {
  select(self).named(result);
}

void TTX_CALL
    Layout::snapshot_abi(ttx_layout self, ttx_layout_snapshot_result result) {
  select(self).snapshot(result);
}

void TTX_CALL Layout::fluid_abi(ttx_layout self, ttx_fluid_result result) {
  select(self).fluid(result);
}

void TTX_CALL
    Layout::value_abi(ttx_layout self, ttx_value_layout_result result) {
  select(self).value(result);
}

void TTX_CALL
    Layout::composite_abi(ttx_layout self, ttx_composite_layout_result result) {
  select(self).composite(result);
}

void TTX_CALL
    Layout::ranged_abi(ttx_layout self, ttx_ranged_layout_result result) {
  select(self).ranged(result);
}

void TTX_CALL
    Layout::reindexed_abi(ttx_layout self, ttx_reindexed_layout_result result) {
  select(self).reindexed(result);
}
