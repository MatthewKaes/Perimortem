// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/utility/range.hpp"

namespace Tetrodotoxin::Compiler::Execution {

// A contiguous straight-line range of operations ending in a terminator.
class Block {
 public:
  explicit constexpr Block(Perimortem::Utility::Range operations)
      : operations(operations) {}

  constexpr auto get_operations() const -> Perimortem::Utility::Range {
    return operations;
  }

 private:
  Perimortem::Utility::Range operations;
};

}  // namespace Tetrodotoxin::Compiler::Execution
