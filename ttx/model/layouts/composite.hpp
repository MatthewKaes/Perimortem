// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "ttx/concept/layout.hpp"

namespace Ttx::Model::Layouts {

// Composite describes positional composition without copying Abstract edges.
// It borrows two complete Layouts and presents their entries as one ordered
// shape. Indexing selects the owning child while fitting delegates each target
// segment to that child's concrete fitting rules. Composite can therefore keep
// a compact Ranged prefix beside a Fluid suffix and recursively compose more
// than two Layouts without becoming the Pack or Type that exposes it.
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

  constexpr auto get_name(Count index) const
      -> Perimortem::Core::Option<Perimortem::Core::View::Bytes> override {
    if (index < first.get_size()) {
      return first.get_name(index);
    }

    if (index >= get_size()) {
      return {};
    }

    return second.get_name(index - first.get_size());
  }

  auto fits_entry(
      const Concept::Layout& target,
      Count source_index,
      Count target_index) const -> Bool override;

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
