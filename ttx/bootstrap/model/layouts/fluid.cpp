// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "ttx/bootstrap/model/layouts/fluid.hpp"

#include "ttx/bootstrap/model/addressable.hpp"

using namespace Ttx;
using namespace Ttx::Model;

static auto resolves_for_fitting(const Concept::Abstract& value)
    -> const Concept::Abstract& {
  // A reserved Type is already the exact semantic fact needed by Layout
  // negotiation even when its owner has not completed resolution yet. Resolving
  // it first would collapse every incomplete Type to the shared Unknown object
  // and make unrelated staged identities appear compatible.
  if (value.is<Type>()) {
    return value;
  }

  auto direct_addressable = value.select<Addressable>();
  if (direct_addressable) {
    return direct_addressable->get_type();
  }

  const Concept::Abstract& represented = value.resolve();
  return represented.visit<Addressable>(
      [](const Addressable& addressable) -> const Concept::Abstract& {
        return addressable.get_type();
      },
      [](const Concept::Abstract& abstract) -> const Concept::Abstract& {
        return abstract;
      });
}

static auto fits_entry(
    const Concept::Abstract& source,
    const Concept::Abstract& target) -> Bool {
  return &resolves_for_fitting(source) == &target ? True : False;
}

auto Layouts::Fluid::fits_entry(
    const Concept::Layout& target,
    Count source_index,
    Count target_index) const -> Bool {
  BAIL_IF(source_index >= get_size() || target_index >= target.get_size());

  const Concept::Abstract& source = *abstracts.get_data()[source_index];
  return target.get_abstract(target_index)
      .visit(
          []() { return False; },
          [&](const Concept::Abstract& target_entry) {
            return ::fits_entry(source, resolves_for_fitting(target_entry));
          });
}

auto Layouts::Fluid::fits_at(const Concept::Layout& target, Count target_offset)
    const -> Bool {
  if (!has_target_segment(target, target_offset)) {
    return False;
  }

  for (Count i = 0; i < get_size(); i++) {
    if (!fits_entry(target, i, target_offset + i)) {
      return False;
    }
  }

  return True;
}

auto Layouts::Fluid::get_fitted_at(
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

  return *abstracts.get_data()[target_index];
}
