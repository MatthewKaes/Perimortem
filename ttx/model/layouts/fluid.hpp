// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/vector.hpp"

#include "ttx/model/layout.hpp"

namespace Ttx::Model::Layouts {

// Fluid is positional value flow. Its entries are the real Abstracts produced
// by an expression, pack, return, or other reshapeable source. It fits another
// Layout by ordered resolved identity and carries no field metadata.
class Fluid : public Layout {
 public:
  Fluid(
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
  Perimortem::Core::View::Vector<const Abstraction::Abstract*> abstracts;
};

}  // namespace Ttx::Model::Layouts
