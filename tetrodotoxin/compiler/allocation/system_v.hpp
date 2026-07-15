// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/vector.hpp"

#include "perimortem/memory/dynamic/vector.hpp"

#include "ttx/member.hpp"

namespace Tetrodotoxin::Compiler::Allocation {

// Classifies one parameter layout for the x86-64 System V calling convention.
//
// TTX currently lowers values to one or two eight-byte components. Integer and
// pointer components consume the six general argument registers, while real
// components consume the eight SSE registers. If every component of a value
// does not fit in its register bank, the whole value receives consecutive
// stack slots. Keeping that decision at value granularity matches the System V
// rollback rule and prevents a byte view from being split between registers
// and the stack.
//
// Planning is linear in the number of parameters and ABI components. It does
// not search alternative assignments because System V fixes both register
// order and stack order. A value that cannot fit leaves its register bank
// untouched, allowing a later narrower value to use the remaining register.
//
// This planner records declaration-order stack slots. A callee reads those
// slots in that order, while a caller emits them in reverse. It does not yet
// implement the full eightbyte classification for mixed aggregates, memory
// class returns, or vector classes because the current TTX ABI exposes only
// scalar values and byte views. Those features should extend this planner
// before instruction lowering learns another set of classification rules.
class SystemV {
 public:
  enum class Bank : Bits_8 {
    Integer,
    Real,
    Stack,
  };

  struct Location {
    Bank bank;
    Count index;
  };

  explicit SystemV(Perimortem::Core::View::Vector<Ttx::Member> parameters);

  auto get(Count parameter, Count component) const -> Location;
  constexpr auto get_stack_count() const -> Count { return stack_count; }

  static auto get_width(const Ttx::Type& type) -> Count;

 private:
  Perimortem::Memory::Dynamic::Vector<Location> locations;
  Perimortem::Memory::Dynamic::Vector<Count> starts;
  Perimortem::Memory::Dynamic::Vector<Count> widths;
  Count stack_count = 0;
};

}  // namespace Tetrodotoxin::Compiler::Allocation
