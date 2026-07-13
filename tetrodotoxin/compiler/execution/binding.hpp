// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "ttx/type.hpp"

namespace Tetrodotoxin::Compiler::Execution {

// The type and first live operation of one Addressable.
class Binding {
 public:
  constexpr Binding(const Ttx::Type& type, Count start)
      : type(type), start(start) {}

  constexpr auto get_type() const -> const Ttx::Type& { return type; }

  constexpr auto get_start() const -> Count { return start; }

 private:
  const Ttx::Type& type;
  Count start;
};

}  // namespace Tetrodotoxin::Compiler::Execution
