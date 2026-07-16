// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/vector.hpp"

#include "ttx/model/layout.hpp"

namespace Ttx::Model::Layouts {

// Named is reshapeable value flow whose Abstracts author non-empty names.
// Names affect fitting but do not make the values addressable storage. The
// source owner supplies real Abstract objects whose get_name() and resolve()
// queries carry the complete fitting facts.
class Named : public Layout {
 public:
  Named(
      Perimortem::Core::View::Vector<const Abstraction::Abstract*> abstracts =
          {})
      : abstracts(abstracts) {}

  auto get_size() const -> Count override { return abstracts.get_size(); }
  auto get_abstract(Count index) const
      -> const Abstraction::Abstract& override {
    return *abstracts[index];
  }

  auto fits(const Layout& target) const -> Bool override;

 private:
  auto has_unique_names() const -> Bool;

  Perimortem::Core::View::Vector<const Abstraction::Abstract*> abstracts;
};

}  // namespace Ttx::Model::Layouts
