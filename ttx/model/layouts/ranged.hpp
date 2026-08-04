// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "ttx/concept/layout.hpp"

namespace Ttx::Model::Layouts {

// Ranged is a compact homogeneous Layout. It stores one real Abstract and a
// count, then materializes the same semantic answer for every valid index.
// Bytes[N], Static::Vector<T, N>, and other fixed homogeneous Types can
// therefore participate in recursive layout and generalized element
// projection without allocating N duplicate graph edges. An index outside the
// range returns None.
class Ranged : public Concept::Layout {
 public:
  constexpr Ranged(const Concept::Abstract& abstract, Count size)
      : abstract(abstract), size(size) {}

  constexpr auto get_size() const -> Count override { return size; }

  constexpr auto get_abstract(Count index) const
      -> Perimortem::Utility::Option<const Concept::Abstract&> override {
    if (index >= size) {
      return {};
    }

    return abstract;
  }

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
