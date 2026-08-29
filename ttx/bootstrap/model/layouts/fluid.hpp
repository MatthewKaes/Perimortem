// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/view/vector.hpp"

#include "ttx/bootstrap/concept/layout.hpp"

namespace Ttx::Model::Layouts {

// Fluid is the positional descriptor exposed by a Pack or another ordered
// source. Its entries are the real produced Abstracts, so fitting preserves
// their provenance without turning the descriptor into value flow identity.
// Fluid fits another Layout by ordered represented identity and carries no
// field metadata.
class Fluid : public Concept::Layout {
 public:
  constexpr Fluid(
      Perimortem::Core::View::Vector<const Concept::Abstract*> abstracts = {})
      : abstracts(abstracts) {}

  constexpr auto get_size() const -> Count override {
    return abstracts.get_size();
  }
  constexpr auto get_abstract(Count index) const
      -> Perimortem::Core::Option<const Concept::Abstract&> override {
    if (index >= abstracts.get_size()) {
      return {};
    }

    return *abstracts.get_data()[index];
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
  Perimortem::Core::View::Vector<const Concept::Abstract*> abstracts;
};

}  // namespace Ttx::Model::Layouts
