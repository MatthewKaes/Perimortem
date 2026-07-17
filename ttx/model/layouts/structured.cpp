// Perimortem Engine
// Copyright © Matt Kaes

#include "ttx/model/layouts/structured.hpp"

auto Ttx::Model::Layouts::Structured::fits(const Concept::Layout& target) const
    -> Bool {
  if (get_size() != target.get_size()) {
    return False;
  }

  for (Count i = 0; i < get_size(); i++) {
    if (&get_abstract(i) != &target.get_abstract(i)) {
      return False;
    }
  }

  return True;
}

auto Ttx::Model::Layouts::Structured::get_fitted(
    const Concept::Layout& target,
    Count target_index) const -> const Concept::Abstract& {
  if (!fits(target) || target_index >= target.get_size()) {
    return invalid_result;
  }

  return get_abstract(target_index);
}
