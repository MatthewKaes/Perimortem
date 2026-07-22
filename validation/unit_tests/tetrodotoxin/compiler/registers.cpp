// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/compiler/allocation/registers.hpp"

#include "validation/unit_test.hpp"

#include "perimortem/core/static/vector.hpp"

#include "perimortem/memory/allocator/arena.hpp"

#include "tetrodotoxin/model/constants/unsigned.hpp"
#include "ttx/concept/reference.hpp"
#include "ttx/model/bodies/builder.hpp"
#include "ttx/model/layouts/fluid.hpp"
#include "ttx/model/types/unsigned_64.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Ttx::Concept;
using namespace Ttx::Model;
using namespace Ttx::Model::Bodies;
using namespace Ttx::Model::Layouts;
using namespace Tetrodotoxin::Compiler;
using namespace Validation;

static Harness CompilerRegisters = {
  .name = "Compiler::Registers"_view,
};

PERIMORTEM_UNIT_TEST(CompilerRegisters, reuses_non_overlapping_colors) {
  Allocator::Arena arena;
  Types::Unsigned_64 type;
  Tetrodotoxin::Model::Constants::Unsigned first(type, 1);
  Tetrodotoxin::Model::Constants::Unsigned second(type, 2);
  Fluid parameters;
  Fluid results;
  Builder builder(arena, parameters, results);

  ASSERT(builder.constant(first).is_valid());
  ASSERT(builder.constant(second).is_valid());
  ASSERT(builder.return_values());
  Body body = builder.finish();
  const Static::Vector<Count, 2> widths = {{1, 1}};
  Allocation::Registers allocation(body, widths, 1);

  EXPECT_EQ(allocation.get_color(0), Count(0));
  EXPECT_EQ(allocation.get_color(1), Count(0));
  EXPECT_EQ(allocation.get_spill_count(), Count(0));
}

PERIMORTEM_UNIT_TEST(CompilerRegisters, extends_ranges_through_uses) {
  Allocator::Arena arena;
  Types::Unsigned_64 type;
  Tetrodotoxin::Model::Constants::Unsigned constant(type, 1);
  const Static::Vector<Reference<Abstract>, 1> entries = {{type}};
  Fluid parameters(entries);
  Fluid results(entries);
  Builder builder(arena, parameters, results);

  ASSERT(builder.constant(constant).is_valid());
  const Static::Vector<ValueId, 1> returned = {{ValueId(0)}};
  ASSERT(builder.return_values(returned));
  Body body = builder.finish();
  const Static::Vector<Count, 2> widths = {{1, 1}};
  Allocation::Registers allocation(body, widths, 1);

  EXPECT_EQ(allocation.get_color(0), Count(0));
  EXPECT_EQ(allocation.get_color(1), Count(-1));
  EXPECT_EQ(allocation.get_spill(1), Count(0));
  EXPECT_EQ(allocation.get_spill_count(), Count(1));
}

PERIMORTEM_UNIT_TEST(CompilerRegisters, keeps_components_consecutive) {
  Allocator::Arena arena;
  Types::Unsigned_64 type;
  Tetrodotoxin::Model::Constants::Unsigned first(type, 1);
  Tetrodotoxin::Model::Constants::Unsigned second(type, 2);
  const Static::Vector<Reference<Abstract>, 2> entries = {{type, type}};
  Fluid parameters;
  Fluid results(entries);
  Builder builder(arena, parameters, results);

  ValueId first_value = builder.constant(first);
  ValueId second_value = builder.constant(second);
  ASSERT(first_value.is_valid());
  ASSERT(second_value.is_valid());
  const Static::Vector<ValueId, 2> returned = {{first_value, second_value}};
  ASSERT(builder.return_values(returned));
  Body body = builder.finish();
  const Static::Vector<Count, 2> widths = {{2, 2}};
  Allocation::Registers allocation(body, widths, 3);

  EXPECT_EQ(allocation.get_color(0, 0), Count(0));
  EXPECT_EQ(allocation.get_color(0, 1), Count(1));
  EXPECT_EQ(allocation.get_color(1), Count(-1));
  EXPECT_EQ(allocation.get_spill(1, 0), Count(0));
  EXPECT_EQ(allocation.get_spill(1, 1), Count(1));
  EXPECT_EQ(allocation.get_spill_count(), Count(2));
}
