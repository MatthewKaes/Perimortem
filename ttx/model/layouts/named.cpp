// Perimortem Engine
// Copyright © Matt Kaes

#include "ttx/model/layouts/named.hpp"

auto Ttx::Model::Layouts::Named::has_unique_names() const -> Bool {
  for (Count i = 0; i < get_size(); i++) {
    Perimortem::Core::View::Bytes name = get_abstract(i).get_name();
    if (name.is_empty()) {
      return False;
    }

    for (Count other = i + 1; other < get_size(); other++) {
      if (name == get_abstract(other).get_name()) {
        return False;
      }
    }
  }

  return True;
}

auto Ttx::Model::Layouts::Named::fits(const Layout& target) const -> Bool {
  if (get_size() != target.get_size() || !has_unique_names()) {
    return False;
  }

  for (Count i = 0; i < get_size(); i++) {
    const Abstraction::Abstract& source = get_abstract(i);
    Count matches = 0;
    for (Count target_index = 0; target_index < target.get_size();
         target_index++) {
      const Abstraction::Abstract& candidate =
          target.get_abstract(target_index);
      if (source.get_name() != candidate.get_name()) {
        continue;
      }

      if (&source.resolve() != &candidate.resolve()) {
        return False;
      }

      matches++;
    }

    if (matches != 1) {
      return False;
    }
  }

  return True;
}
