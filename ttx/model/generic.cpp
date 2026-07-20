// Perimortem Engine
// Copyright © Matt Kaes

#include "ttx/model/generic.hpp"

#include "ttx/model/constant.hpp"

auto Ttx::Model::Generic::arguments_equal(
    const Concept::Layout& lhs,
    const Concept::Layout& rhs) -> Bool {
  if (lhs.get_size() != rhs.get_size()) {
    return False;
  }

  for (Count i = 0; i < lhs.get_size(); i++) {
    const Concept::Abstract& lhs_argument = lhs.get_abstract(i).resolve();
    const Concept::Abstract& rhs_argument = rhs.get_abstract(i).resolve();
    if (&lhs_argument == &rhs_argument) {
      continue;
    }
    if (!lhs_argument.is<Constant>() || !rhs_argument.is<Constant>() ||
        lhs_argument.assume<Constant>() != rhs_argument.assume<Constant>()) {
      return False;
    }
  }

  return True;
}
