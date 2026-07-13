// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/vector.hpp"

#include "perimortem/memory/dynamic/vector.hpp"

#include "tetrodotoxin/compiler/execution/body.hpp"

namespace Tetrodotoxin::Compiler::Allocation {

// Assigns abstract register colors and spill slots to one immutable Body.
//
// The allocator first derives a conservative half-open live interval for each
// SSA binding. It then visits bindings in their stable definition order and
// chooses the first consecutive color range that does not intersect an earlier
// live value. Values that cannot fit receive consecutive spill slots. A width
// greater than one keeps compound ABI values, such as a byte view represented
// by pointer and length, together without teaching this pass about target
// types.
//
// This is deliberately a small straight-line allocator. It is deterministic,
// target-independent, and inexpensive for the short functions the current
// execution model can express. It does not split intervals, reuse spill slots,
// model call clobbers, or optimize move affinity. Those policies belong in a
// later allocator once Execution grows control flow and supplies block-level
// liveness.
class Registers {
 public:
  Registers(
      const Execution::Body& body,
      Perimortem::Core::View::Vector<Count> widths,
      Count color_count);

  auto get_color(Count binding, Count component = 0) const -> Count;
  auto get_spill(Count binding, Count component = 0) const -> Count;
  constexpr auto get_spill_count() const -> Count { return spill_count; }

 private:
  Perimortem::Memory::Dynamic::Vector<Count> colors;
  Perimortem::Memory::Dynamic::Vector<Count> spills;
  Perimortem::Memory::Dynamic::Vector<Count> widths;
  Count spill_count = 0;
};

}  // namespace Tetrodotoxin::Compiler::Allocation
