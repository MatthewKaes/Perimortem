// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/vector.hpp"

#include "ttx/concept/layout.hpp"
#include "ttx/concept/reference.hpp"

namespace Ttx::Model::Layouts {

// Fluid is positional value flow. Its entries are the real Abstracts produced
// by an expression, pack, return, or other reshapeable source. It fits another
// Layout by ordered resolved identity and carries no field metadata. An
// Expression entry instead proves that it fits the resolved target Type, which
// keeps value-domain conversion knowledge out of Layout.
class Fluid : public Concept::Layout {
 public:
  constexpr Fluid(
      Perimortem::Core::View::Vector<Concept::Reference<Concept::Abstract>>
          abstracts = {})
      : abstracts(abstracts) {}

  constexpr auto get_size() const -> Count override {
    return abstracts.get_size();
  }
  constexpr auto get_abstract(Count index) const
      -> const Concept::Abstract& override {
    if (index >= abstracts.get_size()) {
      return Concept::Invalid::get_invalid();
    }

    return abstracts[index].get();
  }

  auto fits(const Concept::Layout& target) const -> Bool override;
  auto get_fitted(const Concept::Layout& target, Count target_index) const
      -> const Concept::Abstract& override;

 private:
  Perimortem::Core::View::Vector<Concept::Reference<Concept::Abstract>>
      abstracts;
};

}  // namespace Ttx::Model::Layouts
