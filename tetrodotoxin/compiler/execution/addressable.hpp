// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/perimortem.hpp"

namespace Tetrodotoxin::Compiler::Execution {

// A stable SSA value id. Binding owns its type and live range. The backend owns
// its physical location.
class Addressable {
 public:
  constexpr Addressable() = delete;
  explicit constexpr Addressable(Count id) : id(id) {}

  constexpr auto get_id() const -> Count { return id; }

 private:
  Count id;
};

}  // namespace Tetrodotoxin::Compiler::Execution
