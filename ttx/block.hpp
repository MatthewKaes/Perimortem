// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/data.hpp"

namespace Ttx {

class Type;

// Block is the stable handle for an externally represented executable body.
//
// TTX functions need to know that a body exists, but statement shape belongs to
// the layer that evaluated the function. The core model therefore stores only
// the representation Type for the body. Consumers that know that representation
// can cast the block back to the richer body object they own.
class Block {
 public:
  constexpr Block() = default;
  explicit constexpr Block(const Type& representation)
      : representation(&representation) {}

  constexpr auto get_representation() const -> const Type* {
    return representation;
  }
  constexpr auto is_empty() const -> Bool { return representation == nullptr; }

 private:
  const Type* representation = nullptr;
};

}  // namespace Ttx
