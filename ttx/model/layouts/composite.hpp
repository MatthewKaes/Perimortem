// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "ttx/concept/layout.hpp"

namespace Ttx::Model::Layouts {

// Composite is positional composition without copied Abstract edges. It
// borrows two complete Layouts and presents their entries as one ordered shape.
// Indexing selects the owning child while fitting delegates each target segment
// to that child's concrete fitting rules. Composite can therefore retain a
// compact Ranged prefix beside a Fluid suffix and can recursively compose more
// than two Layouts without flattening them.
class Composite : public Concept::Layout {
 public:
  constexpr Composite(
      const Concept::Layout& first,
      const Concept::Layout& second)
      : first(first), second(second) {}

  constexpr auto get_size() const -> Count override {
    return first.get_size() + second.get_size();
  }

  constexpr auto get_abstract(Count index) const
      -> Perimortem::Core::Option<const Concept::Abstract&> override {
    if (index < first.get_size()) {
      return first.get_abstract(index);
    }

    if (index >= get_size()) {
      return {};
    }

    return second.get_abstract(index - first.get_size());
  }

  auto fits_at(const Concept::Layout& target, Count target_offset) const
      -> Bool override;
  auto get_fitted_at(
      const Concept::Layout& target,
      Count target_offset,
      Count target_index) const
      -> Perimortem::Utility::Result<const Concept::Abstract&, Errors> override;

 private:
  const Concept::Layout& first;
  const Concept::Layout& second;
};

}  // namespace Ttx::Model::Layouts
