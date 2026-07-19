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
// resolved Type, documentation, attributes, defaults, and ISA facts therefore
// remain queryable on their real owner without becoming Layout fields.
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
      -> const Concept::Abstract& override {
    if (index >= addressables.get_size()) {
      return Concept::Invalid::get_invalid();
    }

    return addressables[index].get();
  }

  constexpr auto fits(const Concept::Layout& target) const -> Bool override {
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

  constexpr auto get_fitted(const Concept::Layout& target, Count target_index)
      const -> const Concept::Abstract& override {
    if (!fits(target) || target_index >= target.get_size()) {
      return Concept::Invalid::get_invalid();
    }

    return get_abstract(target_index);
  }

 private:
  Perimortem::Core::View::Vector<Concept::Reference<Addressable>> addressables;
};

}  // namespace Ttx::Model::Layouts
