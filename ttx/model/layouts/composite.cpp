// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "ttx/model/layouts/composite.hpp"

using namespace Ttx;
using namespace Ttx::Model;

auto Layouts::Composite::fits_entry(
    const Concept::Layout& target,
    Count source_index,
    Count target_index) const -> Bool {
  BAIL_IF(source_index >= get_size() || target_index >= target.get_size());

  return source_index < first.get_size()
             ? first.fits_entry(target, source_index, target_index)
             : second.fits_entry(
                   target, source_index - first.get_size(), target_index);
}

auto Layouts::Composite::fits_at(
    const Concept::Layout& target,
    Count target_offset) const -> Bool {
  if (!has_target_segment(target, target_offset)) {
    return False;
  }

  return first.fits_at(target, target_offset) &&
         second.fits_at(target, target_offset + first.get_size());
}

auto Layouts::Composite::get_fitted_at(
    const Concept::Layout& target,
    Count target_offset,
    Count target_index) const
    -> Perimortem::Utility::Result<const Concept::Abstract&, Errors> {
  if (target_index >= get_size()) {
    return Errors::IndexOutOfBounds;
  }

  if (!has_target_segment(target, target_offset)) {
    return Errors::SizeMismatch;
  }

  if (!fits_at(target, target_offset)) {
    return Errors::IncompatibleFit;
  }

  if (target_index < first.get_size()) {
    return first.get_fitted_at(target, target_offset, target_index);
  }

  return second.get_fitted_at(
      target, target_offset + first.get_size(),
      target_index - first.get_size());
}
