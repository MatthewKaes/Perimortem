// Perimortem Engine
// Copyright © Matt Kaes

#include "ttx/model/layouts/fluid.hpp"

auto Ttx::Model::Layouts::Fluid::fits(const Layout& target) const -> Bool {
  if (get_size() != target.get_size()) {
    return False;
  }

  for (Count i = 0; i < get_size(); i++) {
    if (&get_abstract(i).resolve() != &target.get_abstract(i).resolve()) {
      return False;
    }
  }

  return True;
}
