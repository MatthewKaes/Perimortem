// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/vector.hpp"

#include "ttx/concept/layout.hpp"
#include "ttx/concept/reference.hpp"
#include "ttx/model/addressable.hpp"

namespace Ttx::Model::Layouts {

// Structured is the stable shape of a Type. Every entry is the actual
// Addressable object in the semantic DAG, not a copied member record. Its name,
// resolved Type, documentation, attributes, defaults, and Dialect facts
// therefore remain queryable on their real owner without becoming Layout
// fields.
class Structured : public Concept::Layout {
 public:
  constexpr Structured(
      Perimortem::Core::View::Vector<Concept::Reference<Addressable>>
          addressables = {})
      : addressables(addressables) {}

  constexpr auto get_size() const -> Count override {
    return addressables.get_size();
  }
  constexpr auto get_abstract(Count index) const
      -> Perimortem::Utility::Option<const Concept::Abstract&> override {
    if (index >= addressables.get_size()) {
      return {};
    }

    return addressables[index].get();
  }

  constexpr auto fits_at(const Concept::Layout& target, Count target_offset)
      const -> Bool override {
    if (!has_target_segment(target, target_offset)) {
      return False;
    }

    for (Count i = 0; i < get_size(); i++) {
      const Addressable& source = addressables[i].get();
      Bool matches = target.get_abstract(target_offset + i)
                         .visit(
                             []() { return False; },
                             [&source](const Concept::Abstract& candidate) {
                               return &source == &candidate ? True : False;
                             });
      if (!matches) {
        return False;
      }
    }

    return True;
  }

  constexpr auto get_fitted_at(
      const Concept::Layout& target,
      Count target_offset,
      Count target_index) const -> Perimortem::Utility::
      Result<const Concept::Abstract&, Errors> override {
    if (target_index >= get_size()) {
      return Errors::IndexOutOfBounds;
    }

    if (!has_target_segment(target, target_offset)) {
      return Errors::SizeMismatch;
    }

    if (!fits_at(target, target_offset)) {
      return Errors::IncompatibleFit;
    }

    return addressables[target_index].get();
  }

 private:
  Perimortem::Core::View::Vector<Concept::Reference<Addressable>> addressables;
};

}  // namespace Ttx::Model::Layouts
