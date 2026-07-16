// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "ttx/abstraction/abstract.hpp"

namespace Ttx::Abstraction {

// Reference is a non-null borrowed semantic edge. It exists because C++
// references cannot be stored directly in contiguous Views or tagged Unions,
// while raw pointers would make an absent Abstract representable again.
//
// Reference preserves the object it receives. Alias and expression resolution
// remain explicit operations owned by the consumer that needs canonical
// identity. The graph owner guarantees the borrowed object's lifetime.
template <typename Contract>
class Reference {
  static_assert(__is_base_of(Abstract, Contract));

 public:
  constexpr Reference(const Contract& abstract) : abstract(abstract) {}

  constexpr auto get() const -> const Contract& { return abstract; }

  constexpr auto operator==(const Reference& rhs) const -> Bool {
    return &abstract == &rhs.abstract;
  }

  constexpr auto operator!=(const Reference& rhs) const -> Bool {
    return !(*this == rhs);
  }

 private:
  const Contract& abstract;
};

}  // namespace Ttx::Abstraction
