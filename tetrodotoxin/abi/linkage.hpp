// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"

#include "perimortem/memory/allocator/arena.hpp"

#include "ttx/type.hpp"

namespace Tetrodotoxin::Abi {

// Joins one canonical TTX function to the machine symbol that implements it.
//
// Function deliberately has no back pointer to its owning Type. The producer
// already knows the owner and selected function table while publishing the
// callable, so Linkage carries that relationship forward without asking a later
// consumer to reconstruct it. Calling convention and register placement remain
// target decisions.
class Linkage {
 public:
  constexpr Linkage(
      const Ttx::Type& owner,
      const Ttx::Function& function,
      Perimortem::Core::View::Bytes symbol)
      : owner(owner), function(function), symbol(symbol) {}

  static auto type(
      Perimortem::Memory::Allocator::Arena& arena,
      const Ttx::Type& owner,
      const Ttx::Function& function,
      Perimortem::Core::View::Bytes unit,
      Perimortem::Core::View::Bytes module,
      Perimortem::Core::View::Bytes owner_path) -> Linkage;
  static auto addressable(
      Perimortem::Memory::Allocator::Arena& arena,
      const Ttx::Type& owner,
      const Ttx::Function& function,
      Perimortem::Core::View::Bytes unit,
      Perimortem::Core::View::Bytes module,
      Perimortem::Core::View::Bytes owner_path) -> Linkage;

  constexpr auto get_owner() const -> const Ttx::Type& { return owner; }
  constexpr auto get_function() const -> const Ttx::Function& {
    return function;
  }

  constexpr auto get_symbol() const -> Perimortem::Core::View::Bytes {
    return symbol;
  }

 private:
  const Ttx::Type& owner;
  const Ttx::Function& function;
  Perimortem::Core::View::Bytes symbol;
};

}  // namespace Tetrodotoxin::Abi
