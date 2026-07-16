// Perimortem Engine
// Copyright © Matt Kaes

#include "ttx/model/layouts/fluid.hpp"

#include "ttx/model/expression.hpp"

auto Ttx::Model::Layouts::Fluid::fits(const Layout& target) const -> Bool {
  if (get_size() != target.get_size()) {
    return False;
  }

  for (Count i = 0; i < get_size(); i++) {
    const Abstraction::Abstract& source = get_abstract(i);
    const Abstraction::Abstract& target_type = target.get_abstract(i).resolve();
    if (source.is<Expression>()) {
      if (!target_type.is<Type>() ||
          !source.as<Expression>().fits(target_type.as<Type>())) {
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

auto Ttx::Model::Layouts::Fluid::get_fitted(
    const Layout& target,
    Count target_index) const -> const Abstraction::Abstract& {
  if (!fits(target) || target_index >= target.get_size()) {
    return invalid_result;
  }

  return get_abstract(target_index);
}
