// Perimortem Engine
// Copyright © Matt Kaes

#include "ttx/model/layouts/ranged.hpp"

#include "ttx/model/addressable.hpp"

using namespace Ttx;
using namespace Ttx::Model;

static auto resolves_for_fitting(const Concept::Abstract& value)
    -> const Concept::Abstract& {
  // Direct staged identities remain valid Layout facts before their lifecycle
  // owners can make resolve() succeed. Preserve them so two unrelated
  // incomplete Types never compare as the same shared Invalid identity.
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

auto Layouts::Ranged::fits_entry(
    const Concept::Layout& target,
    Count source_index,
    Count target_index) const -> Bool {
  BAIL_IF(source_index >= get_size() || target_index >= target.get_size());

  return target.get_abstract(target_index)
      .visit(
          []() { return False; },
          [&](const Concept::Abstract& target_entry) {
            return ::fits_entry(abstract, resolves_for_fitting(target_entry));
          });
}

auto Layouts::Ranged::fits_at(
    const Concept::Layout& target,
    Count target_offset) const -> Bool {
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

auto Layouts::Ranged::get_fitted_at(
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

  return abstract;
}
