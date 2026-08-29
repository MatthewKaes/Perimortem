// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "ttx/concept/layout.hpp"

namespace Ttx::Model {

// Pack is the host-neutral support value for one named semantic flow. Its
// Layout points directly at the real Abstracts that supply the flow, so Pack
// introduces no identity, producer map, or second graph.
class Pack : public Concept::Pack {
 public:
  virtual constexpr auto get_layout() const -> const Concept::Layout& = 0;
};

// PackReference is the assignable nonnull borrow used when an identity-free
// Pack must live in a container. It retains no graph identity and exposes no
// resolution operation.
template <typename Category>
class PackReference {
  static_assert(__is_base_of(Pack, __remove_cvref(Category)));

 public:
  constexpr PackReference(Category& pack) : pack(&pack) {}

  constexpr auto get() const -> Category& { return *pack; }

  constexpr auto operator==(const PackReference& rhs) const -> Bool {
    return pack == rhs.pack;
  }

  constexpr auto operator!=(const PackReference& rhs) const -> Bool {
    return !(*this == rhs);
  }

 private:
  Category* pack;
};

}  // namespace Ttx::Model
