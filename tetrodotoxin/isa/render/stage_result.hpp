// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "ttx/type.hpp"

namespace Tetrodotoxin::Isa::Render {

class StageResult {
 public:
  constexpr StageResult() = default;
  constexpr StageResult(Ttx::Function function, const Ttx::Type& facts)
      : function(function), facts(&facts) {}

  constexpr auto get_function() const -> Ttx::Function { return function; }
  constexpr auto get_facts() const -> const Ttx::Type& { return *facts; }
  constexpr auto is_empty() const -> Bool {
    return function.is_empty() || facts == nullptr;
  }

 private:
  Ttx::Function function;
  const Ttx::Type* facts = nullptr;
};

}  // namespace Tetrodotoxin::Isa::Render
