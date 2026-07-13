// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"

#include "perimortem/memory/allocator/arena.hpp"

#include "ttx/type.hpp"

namespace Tetrodotoxin::Compiler {

// Linkage joins one canonical TTX function to its published symbol.
//
// Function deliberately has no back pointer to its owning Type. Linkage keeps
// both references because the package format identifies a function by its
// owner and owner-local function index. Calling convention and register
// placement remain backend decisions.
class Linkage {
 public:
  constexpr Linkage(
      const Ttx::Type& owner,
      const Ttx::Function& function,
      Perimortem::Core::View::Bytes symbol)
      : owner(owner), function(function), symbol(symbol) {}

  static auto internal(
      Perimortem::Memory::Allocator::Arena& arena,
      const Ttx::Type& owner,
      const Ttx::Function& function,
      Perimortem::Core::View::Bytes module) -> Linkage;

  constexpr auto get_owner() const -> const Ttx::Type& { return owner; }
  constexpr auto get_function() const -> const Ttx::Function& {
    return function;
  }

  constexpr auto get_symbol() const -> Perimortem::Core::View::Bytes {
    return symbol;
  }

  constexpr auto is_valid() const -> Bool {
    return !owner.is_invalid() && !function.is_empty() && !symbol.is_empty();
  }

 private:
  const Ttx::Type& owner;
  const Ttx::Function& function;
  Perimortem::Core::View::Bytes symbol;
};

}  // namespace Tetrodotoxin::Compiler
