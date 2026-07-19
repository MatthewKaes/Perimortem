// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/compiler/allocation/registers.hpp"

#include "validation/unit_test.hpp"

#include "tetrodotoxin/compiler/allocation/system_v.hpp"
#include "tetrodotoxin/standard/types.hpp"

using namespace Perimortem::Core;
using namespace Tetrodotoxin::Compiler;
using namespace Validation;

static Harness CompilerRegisters = {
  .name = "Compiler::Registers"_view,
};

PERIMORTEM_UNIT_TEST(CompilerRegisters, reuses_non_overlapping_colors) {
  Ttx::Type type("Value"_view);
  Execution::Binding bindings[] = {
    {type, 0},
    {type, 1},
    {type, 1},
  };
  Count widths[] = {1, 1, 1};
  Execution::Body body({}, {}, {}, bindings);
  Allocation::Registers allocation(body, widths, 2);
  EXPECT_EQ(allocation.get_color(0), Count(0));
  EXPECT_EQ(allocation.get_color(1), Count(0));
  EXPECT_EQ(allocation.get_color(2), Count(1));
  EXPECT_EQ(allocation.get_spill_count(), Count(0));
}

PERIMORTEM_UNIT_TEST(CompilerRegisters, extends_ranges_through_uses) {
  Ttx::Type type("Value"_view);
  Execution::Block blocks[] = {
    Execution::Block({0, 1}),
  };
  Execution::Operation operations[] = {
    Execution::Operation(Execution::Return({0, 1})),
  };
  Execution::Operand operands[] = {
    Execution::Operand(Execution::Addressable(0)),
  };
  Execution::Binding bindings[] = {
    {type, 0},
    {type, 1},
  };
  Count widths[] = {1, 1};
  Execution::Body body(blocks, operations, operands, bindings);
  Allocation::Registers allocation(body, widths, 1);
  EXPECT_EQ(allocation.get_color(0), Count(0));
  EXPECT_EQ(allocation.get_color(1), Count(-1));
  EXPECT_EQ(allocation.get_spill(1), Count(0));
  EXPECT_EQ(allocation.get_spill_count(), Count(1));
}

PERIMORTEM_UNIT_TEST(CompilerRegisters, keeps_components_consecutive) {
  Ttx::Type type("Pair"_view);
  Execution::Binding bindings[] = {
    {type, 0},
    {type, 0},
  };
  Count widths[] = {2, 2};
  Execution::Body body({}, {}, {}, bindings);
  Allocation::Registers allocation(body, widths, 3);
  EXPECT_EQ(allocation.get_color(0, 0), Count(0));
  EXPECT_EQ(allocation.get_color(0, 1), Count(1));
  EXPECT_EQ(allocation.get_color(1), Count(-1));
  EXPECT_EQ(allocation.get_spill(1, 0), Count(0));
  EXPECT_EQ(allocation.get_spill(1, 1), Count(1));
  EXPECT_EQ(allocation.get_spill_count(), Count(2));
}

PERIMORTEM_UNIT_TEST(CompilerRegisters, system_v_parameter_banks) {
  const Ttx::Type& integer =
      *Tetrodotoxin::Standard::Types::find_type("Unsigned_64"_view);
  const Ttx::Type& real =
      *Tetrodotoxin::Standard::Types::find_type("Real_64"_view);
  const Ttx::Type& bytes =
      *Tetrodotoxin::Standard::Types::find_type("View[Bytes]"_view);
  Ttx::Member parameters[] = {
    {"i0"_view, integer}, {"i1"_view, integer}, {"i2"_view, integer},
    {"i3"_view, integer}, {"i4"_view, integer}, {"bytes"_view, bytes},
    {"i5"_view, integer}, {"r0"_view, real},    {"r1"_view, real},
    {"r2"_view, real},    {"r3"_view, real},    {"r4"_view, real},
    {"r5"_view, real},    {"r6"_view, real},    {"r7"_view, real},
    {"r8"_view, real},
  };
  Allocation::SystemV allocation(parameters);

  EXPECT(allocation.get(0, 0).get_bank() == Allocation::SystemVBank::Integer);
  EXPECT_EQ(allocation.get(0, 0).get_index(), Count(0));
  EXPECT(allocation.get(5, 0).get_bank() == Allocation::SystemVBank::Stack);
  EXPECT_EQ(allocation.get(5, 0).get_index(), Count(0));
  EXPECT_EQ(allocation.get(5, 1).get_index(), Count(1));
  EXPECT(allocation.get(6, 0).get_bank() == Allocation::SystemVBank::Integer);
  EXPECT_EQ(allocation.get(6, 0).get_index(), Count(5));
  EXPECT(allocation.get(14, 0).get_bank() == Allocation::SystemVBank::Real);
  EXPECT_EQ(allocation.get(14, 0).get_index(), Count(7));
  EXPECT(allocation.get(15, 0).get_bank() == Allocation::SystemVBank::Stack);
  EXPECT_EQ(allocation.get(15, 0).get_index(), Count(2));
  EXPECT_EQ(allocation.get_stack_count(), Count(3));
}
