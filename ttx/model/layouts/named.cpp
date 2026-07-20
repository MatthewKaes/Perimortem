// Perimortem Engine
// Copyright © Matt Kaes

#include "ttx/model/layouts/named.hpp"

#include "ttx/model/addressable.hpp"
#include "ttx/model/expression.hpp"

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

auto Ttx::Model::Layouts::Named::fits_at(
    const Concept::Layout& target,
    Count target_offset) const -> Bool {
  if (!has_target_segment(target, target_offset) || !has_unique_names()) {
    return False;
  }

  for (Count i = 0; i < get_size(); i++) {
    const Concept::Abstract& source = get_abstract(i);
    Count matches = 0;
    for (Count target_index = 0; target_index < get_size(); target_index++) {
      const Concept::Abstract& candidate =
          target.get_abstract(target_offset + target_index);
      if (source.get_name() != candidate.get_name()) {
        continue;
      }

      const Concept::Abstract& target_type =
          candidate.is<Addressable>()
              ? candidate.assume<Addressable>().get_type().resolve()
              : candidate.resolve();
      if (source.is<Expression>()) {
        if (!target_type.is<Type>() ||
            !source.assume<Expression>().fits(target_type.assume<Type>())) {
          return False;
        }
      } else if (&source.resolve() != &target_type) {
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

auto Ttx::Model::Layouts::Named::get_fitted_at(
    const Concept::Layout& target,
    Count target_offset,
    Count target_index) const -> const Concept::Abstract& {
  if (target_index >= get_size() || !fits_at(target, target_offset)) {
    return Concept::Invalid::get_invalid();
  }

  const Concept::Abstract& requested =
      target.get_abstract(target_offset + target_index);
  for (Count i = 0; i < get_size(); i++) {
    const Concept::Abstract& source = get_abstract(i);
    if (source.get_name() != requested.get_name()) {
      continue;
    }

    const Concept::Abstract& target_type =
        requested.is<Addressable>()
            ? requested.assume<Addressable>().get_type().resolve()
            : requested.resolve();
    if (source.is<Expression>()) {
      if (target_type.is<Type>() &&
          source.assume<Expression>().fits(target_type.assume<Type>())) {
        return source;
      }
    } else if (&source.resolve() == &target_type) {
      return source;
    }
  }

  return Concept::Invalid::get_invalid();
}
