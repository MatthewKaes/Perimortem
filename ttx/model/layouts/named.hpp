// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/vector.hpp"

#include "ttx/concept/layout.hpp"
#include "ttx/concept/reference.hpp"

namespace Ttx::Model::Layouts {

// Named is reshapeable value flow whose Abstracts author names that are not
// empty.
// Names affect fitting but do not make the values addressable storage. The
// source owner supplies real Abstract objects whose get_name() and resolve()
// queries carry the complete fitting facts.
class Named : public Concept::Layout {
 public:
  constexpr Named(
      Perimortem::Core::View::Vector<
          Concept::Reference<const Concept::Abstract>> abstracts = {})
      : abstracts(abstracts) {}

  constexpr auto get_size() const -> Count override {
    return abstracts.get_size();
  }
  constexpr auto get_abstract(Count index) const
      -> Perimortem::Core::Option<const Concept::Abstract&> override {
    if (index >= abstracts.get_size()) {
      return {};
    }

    return abstracts.get_data()[index].get();
  }

  auto fits_at(const Concept::Layout& target, Count target_offset) const
      -> Bool override;
  auto get_fitted_at(
      const Concept::Layout& target,
      Count target_offset,
      Count target_index) const
      -> Perimortem::Utility::Result<const Concept::Abstract&, Errors> override;

 private:
  auto has_unique_names() const -> Bool;

  Perimortem::Core::View::Vector<Concept::Reference<const Concept::Abstract>>
      abstracts;
};

}  // namespace Ttx::Model::Layouts
