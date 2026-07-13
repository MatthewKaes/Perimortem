// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/compiler/allocation/registers.hpp"

#include "validation/unit_test.hpp"

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
