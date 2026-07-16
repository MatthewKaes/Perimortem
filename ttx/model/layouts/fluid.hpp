// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/vector.hpp"

#include "ttx/abstraction/reference.hpp"
#include "ttx/model/layout.hpp"

namespace Ttx::Model::Layouts {

// Fluid is positional value flow. Its entries are the real Abstracts produced
// by an expression, pack, return, or other reshapeable source. It fits another
// Layout by ordered resolved identity and carries no field metadata. An
// Expression entry instead proves that it fits the resolved target Type, which
// keeps value-domain conversion knowledge out of Layout.
class Fluid : public Layout {
 public:
  Fluid(
      Perimortem::Core::View::Vector<
          Abstraction::Reference<Abstraction::Abstract>> abstracts = {})
      : abstracts(abstracts) {}

  auto get_size() const -> Count override { return abstracts.get_size(); }
  auto get_abstract(Count index) const
      -> const Abstraction::Abstract& override {
    if (index >= abstracts.get_size()) {
      return invalid_result;
    }

    return abstracts[index].get();
  }

  auto fits(const Layout& target) const -> Bool override;
  auto get_fitted(const Layout& target, Count target_index) const
      -> const Abstraction::Abstract& override;

 private:
  Perimortem::Core::View::Vector<Abstraction::Reference<Abstraction::Abstract>>
      abstracts;
};

}  // namespace Ttx::Model::Layouts
