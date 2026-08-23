// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "ttx/concept/abstract.hpp"

namespace Ttx::Concept {

// Reference is a nonnull borrowed semantic edge. It exists because C++
// references cannot be stored directly in contiguous Views or tagged Unions,
// while raw pointers would make an absent Abstract representable again.
//
// Reference preserves the object it receives. Alias and expression resolution
// remain explicit operations owned by the consumer that needs canonical
// identity. The graph owner guarantees the borrowed object's lifetime.
template <typename Category>
class Reference {
  static_assert(__is_base_of(Abstract, __remove_cvref(Category)));

 public:
  constexpr Reference(Category& abstract) : abstract(&abstract) {}

  constexpr auto get() const -> Category& { return *abstract; }

  constexpr auto operator==(const Reference& rhs) const -> Bool {
    return abstract == rhs.abstract;
  }

  constexpr auto operator!=(const Reference& rhs) const -> Bool {
    return !(*this == rhs);
  }

 private:
  // The pointer is private storage for an assignable nonnull reference value.
  // Construction requires a real Category and no API exposes nullable state.
  Category* abstract;
};

}  // namespace Ttx::Concept
