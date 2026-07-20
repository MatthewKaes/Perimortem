// Perimortem Engine
// Copyright © Matt Kaes

#include "ttx/model/layouts/fluid.hpp"

#include "ttx/model/addressable.hpp"
#include "ttx/model/expression.hpp"

auto Ttx::Model::Layouts::Fluid::fits_at(
    const Concept::Layout& target,
    Count target_offset) const -> Bool {
  if (!has_target_segment(target, target_offset)) {
    return False;
  }

  for (Count i = 0; i < get_size(); i++) {
    const Concept::Abstract& source = get_abstract(i);
    const Concept::Abstract& target_entry =
        target.get_abstract(target_offset + i);
    const Concept::Abstract& target_type =
        target_entry.is<Addressable>()
            ? target_entry.assume<Addressable>().get_type().resolve()
            : target_entry.resolve();
    if (source.is<Expression>()) {
      if (!target_type.is<Type>() ||
          !source.assume<Expression>().fits(target_type.assume<Type>())) {
        return False;
      }
      continue;
    }

    if (&source.resolve() != &target_type) {
      return False;
    }
  }

  return True;
}

auto Ttx::Model::Layouts::Fluid::get_fitted_at(
    const Concept::Layout& target,
    Count target_offset,
    Count target_index) const -> const Concept::Abstract& {
  if (target_index >= get_size() || !fits_at(target, target_offset)) {
    return Concept::Invalid::get_invalid();
  }

  return get_abstract(target_index);
}
