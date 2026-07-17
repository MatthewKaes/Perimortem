// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/vector.hpp"

#include "ttx/concept/layout.hpp"
#include "ttx/concept/reference.hpp"

namespace Ttx::Model::Layouts {

// Named is reshapeable value flow whose Abstracts author non-empty names.
// Names affect fitting but do not make the values addressable storage. The
// source owner supplies real Abstract objects whose get_name() and resolve()
// queries carry the complete fitting facts. After a name matches, an Expression
// may prove value compatibility against the resolved target Type instead of
// laundering itself into Type identity.
class Named : public Concept::Layout {
 public:
  Named(
      Perimortem::Core::View::Vector<Concept::Reference<Concept::Abstract>>
          abstracts = {})
      : abstracts(abstracts) {}

  auto get_size() const -> Count override { return abstracts.get_size(); }
  auto get_abstract(Count index) const -> const Concept::Abstract& override {
    if (index >= abstracts.get_size()) {
      return invalid_result;
    }

    return abstracts[index].get();
  }

  auto fits(const Concept::Layout& target) const -> Bool override;
  auto get_fitted(const Concept::Layout& target, Count target_index) const
      -> const Concept::Abstract& override;

 private:
  auto has_unique_names() const -> Bool;

  Perimortem::Core::View::Vector<Concept::Reference<Concept::Abstract>>
      abstracts;
};

}  // namespace Ttx::Model::Layouts
