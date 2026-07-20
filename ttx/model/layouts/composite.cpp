// Perimortem Engine
// Copyright © Matt Kaes

#include "ttx/model/layouts/composite.hpp"

auto Ttx::Model::Layouts::Composite::fits_at(
    const Concept::Layout& target,
    Count target_offset) const -> Bool {
  if (!has_target_segment(target, target_offset)) {
    return False;
  }

  return first.fits_at(target, target_offset) &&
         second.fits_at(target, target_offset + first.get_size());
}

auto Ttx::Model::Layouts::Composite::get_fitted_at(
    const Concept::Layout& target,
    Count target_offset,
    Count target_index) const -> const Concept::Abstract& {
  if (target_index >= get_size() || !fits_at(target, target_offset)) {
    return Concept::Invalid::get_invalid();
  }

  if (target_index < first.get_size()) {
    return first.get_fitted_at(target, target_offset, target_index);
  }

  return second.get_fitted_at(
      target, target_offset + first.get_size(),
      target_index - first.get_size());
}
