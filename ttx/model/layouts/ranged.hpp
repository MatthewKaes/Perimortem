// Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "ttx/concept/layout.hpp"

namespace Ttx::Model::Layouts {

// Ranged is a compact homogeneous descriptor. It stores one real Abstract and a
// count, then supplies the same semantic answer for every valid index. A fixed
// homogeneous Type or a ranged Pack can therefore expose complete shape without
// allocating one graph edge per slot. An index outside the range returns None.
class Ranged : public Concept::Layout {
 public:
  constexpr Ranged(const Concept::Abstract& abstract, Count size)
      : abstract(abstract), size(size) {}

  constexpr auto get_size() const -> Count override { return size; }

  constexpr auto get_abstract(Count index) const
      -> Perimortem::Core::Option<const Concept::Abstract&> override {
    if (index >= size) {
      return {};
    }

    return abstract;
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
  const Concept::Abstract& abstract;
  Count size;
};

}  // namespace Ttx::Model::Layouts
