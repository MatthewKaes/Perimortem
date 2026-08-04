// Perimortem Engine
// Copyright © Matt Kaes

#include "ttx/model/layouts/named.hpp"

#include "ttx/model/addressable.hpp"

using namespace Ttx;
using namespace Ttx::Model;

static auto resolves_for_fitting(const Concept::Abstract& value)
    -> const Concept::Abstract& {
  return value.visit<Addressable>(
      [](const Addressable& addressable) -> const Concept::Abstract& {
        return addressable.get_type().resolve();
      },
      [](const Concept::Abstract& abstract) -> const Concept::Abstract& {
        return abstract.resolve();
      });
}

static auto fits_entry(
    const Concept::Abstract& source,
    const Concept::Abstract& target) -> Bool {
  return &source.resolve() == &target ? True : False;
}

auto Layouts::Named::has_unique_names() const -> Bool {
  for (Count i = 0; i < get_size(); i++) {
    Perimortem::Core::View::Bytes name = abstracts[i].get().get_name();
    if (name.is_empty()) {
      return False;
    }

    for (Count other = i + 1; other < get_size(); other++) {
      if (name == abstracts[other].get().get_name()) {
        return False;
      }
    }
  }

  return True;
}

auto Layouts::Named::fits_at(const Concept::Layout& target, Count target_offset)
    const -> Bool {
  if (!has_target_segment(target, target_offset) || !has_unique_names()) {
    return False;
  }

  for (Count i = 0; i < get_size(); i++) {
    const Concept::Abstract& source = abstracts[i].get();
    Count matches = 0;
    for (Count target_index = 0; target_index < get_size(); target_index++) {
      Bool candidate_fits =
          target.get_abstract(target_offset + target_index)
              .visit(
                  []() { return False; },
                  [&source, &matches](const Concept::Abstract& candidate) {
                    if (source.get_name() != candidate.get_name()) {
                      return True;
                    }

                    const Concept::Abstract& target_type =
                        resolves_for_fitting(candidate);
                    if (!fits_entry(source, target_type)) {
                      return False;
                    }

                    matches++;
                    return True;
                  });
      if (!candidate_fits) {
        return False;
      }
    }

    if (matches != 1) {
      return False;
    }
  }

  return True;
}

auto Layouts::Named::get_fitted_at(
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

  return target.get_abstract(target_offset + target_index)
      .visit(
          []()
              -> Perimortem::Utility::Result<const Concept::Abstract&, Errors> {
            return Errors::IncompatibleFit;
          },
          [this](const Concept::Abstract& requested)
              -> Perimortem::Utility::Result<const Concept::Abstract&, Errors> {
            for (Count i = 0; i < get_size(); i++) {
              const Concept::Abstract& source = abstracts[i].get();
              if (source.get_name() != requested.get_name()) {
                continue;
              }

              const Concept::Abstract& target_type =
                  resolves_for_fitting(requested);
              if (fits_entry(source, target_type)) {
                return source;
              }
            }

            return Errors::IncompatibleFit;
          });
}
