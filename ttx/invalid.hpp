// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "ttx/abstract.hpp"

namespace Ttx {

// Invalid is the closed absorbing failure concept in the TTX graph. It is not a
// diagnostic, nullable stand-in, partially constructed semantic object, or a
// subtype of every contract that might fail. The source-owning caller retains
// the route, source range, and explanation while returning this real Abstract.
//
// Both identity and context resolution return the same Invalid. This preserves
// the first failed boundary and prevents secondary queries from manufacturing
// unrelated semantic results. Invalid is stateless and should gain no further
// concepts unless the fundamental Abstract contract changes.
class Invalid final : public Abstract {
 public:
  constexpr auto get_name() const -> Perimortem::Core::View::Bytes override {
    return "Invalid"_view;
  }

  auto resolve() const -> const Abstract& override { return *this; }

  auto resolve_context(Perimortem::Core::View::Bytes) const
      -> const Abstract& override {
    return *this;
  }
};

}  // namespace Ttx
