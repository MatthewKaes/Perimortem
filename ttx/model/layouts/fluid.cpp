// Perimortem Engine
// Copyright © Matt Kaes

#include "ttx/model/layouts/fluid.hpp"

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

auto Layouts::Fluid::fits_at(const Concept::Layout& target, Count target_offset)
    const -> Bool {
  if (!has_target_segment(target, target_offset)) {
    return False;
  }

  for (Count i = 0; i < get_size(); i++) {
    const Concept::Abstract& source = abstracts.get_data()[i].get();
    Bool entry_fits = target.get_abstract(target_offset + i)
                          .visit(
                              []() { return False; },
                              [&source](const Concept::Abstract& target_entry) {
                                const Concept::Abstract& target_type =
                                    resolves_for_fitting(target_entry);
                                return fits_entry(source, target_type);
                              });
    if (!entry_fits) {
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

  return abstracts.get_data()[target_index].get();
}
