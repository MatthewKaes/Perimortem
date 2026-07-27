// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/vector.hpp"

#include "ttx/concept/layout.hpp"
#include "ttx/concept/reference.hpp"

namespace Ttx::Model::Layouts {

// Fluid is positional value flow. Its entries are the real Abstracts produced
// by a pack, return, or other reshapeable source. It fits another Layout by
// ordered resolved identity and carries no field metadata.
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
      -> Perimortem::Utility::Option<const Concept::Abstract&> override {
    if (index >= abstracts.get_size()) {
      return {};
    }

    return abstracts[index].get();
  }

  auto fits_at(const Concept::Layout& target, Count target_offset) const
      -> Bool override;
  auto get_fitted_at(
      const Concept::Layout& target,
      Count target_offset,
      Count target_index) const -> Perimortem::Core::Static::
      Union<const Concept::Abstract&, Errors> override;

 private:
  Perimortem::Core::View::Vector<Concept::Reference<Concept::Abstract>>
      abstracts;
};

}  // namespace Ttx::Model::Layouts
