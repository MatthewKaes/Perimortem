// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "ttx/concept/abstract.hpp"

namespace Ttx::Concept {

// Reference is a non-null borrowed semantic edge. It exists because C++
// references cannot be stored directly in contiguous Views or tagged Unions,
// while raw pointers would make an absent Abstract representable again.
//
// Reference preserves the object it receives. Alias and expression resolution
// remain explicit operations owned by the consumer that needs canonical
// identity. The graph owner guarantees the borrowed object's lifetime.
template <typename Catagory>
class Reference {
  static_assert(__is_base_of(Abstract, Catagory));

 public:
  constexpr Reference(const Catagory& abstract) : abstract(&abstract) {}

  constexpr auto get() const -> const Catagory& { return *abstract; }

  constexpr auto operator==(const Reference& rhs) const -> Bool {
    return abstract == rhs.abstract;
  }

  constexpr auto operator!=(const Reference& rhs) const -> Bool {
    return !(*this == rhs);
  }

 private:
  // The pointer is private storage for an assignable non-null reference value.
  // Construction requires a real Catagory and no API exposes nullable state.
  const Catagory* abstract;
};

}  // namespace Ttx::Concept
