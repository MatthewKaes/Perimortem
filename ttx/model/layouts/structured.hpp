// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/vector.hpp"

#include "ttx/abstraction/reference.hpp"
#include "ttx/model/addressable.hpp"
#include "ttx/model/layout.hpp"

namespace Ttx::Model::Layouts {

// Structured is the stable shape of a Type. Every entry is the actual
// Addressable object in the semantic DAG, not a copied member record. Its name,
// resolved Type, documentation, attributes, defaults, and ISA facts therefore
// remain queryable on their real owner without becoming Layout fields.
class Structured : public Layout {
 public:
  Structured(
      Perimortem::Core::View::Vector<Abstraction::Reference<Addressable>>
          addressables = {})
      : addressables(addressables) {}

  auto get_size() const -> Count override { return addressables.get_size(); }
  auto get_abstract(Count index) const
      -> const Abstraction::Abstract& override {
    if (index >= addressables.get_size()) {
      return invalid_result;
    }

    return addressables[index].get();
  }

  auto fits(const Layout& target) const -> Bool override;
  auto get_fitted(const Layout& target, Count target_index) const
      -> const Abstraction::Abstract& override;

 private:
  Perimortem::Core::View::Vector<Abstraction::Reference<Addressable>>
      addressables;
};

}  // namespace Ttx::Model::Layouts
