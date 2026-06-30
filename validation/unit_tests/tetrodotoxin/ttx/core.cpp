// Perimortem Engine
// Copyright © Matt Kaes

#include "validation/unit_test.hpp"

#include "tetrodotoxin/ttx/core/type.hpp"

using namespace Perimortem::Core;
using namespace Tetrodotoxin;
using namespace Validation;

static Harness TtxCore = {
  .name = "Tetrodotoxin::Ttx::Core"_view,
};

PERIMORTEM_UNIT_TEST(TtxCore, classifies_primitive_types) {
  EXPECT(Tetrodotoxin::Ttx::Core::is_type("Real_32"_view));
  EXPECT(Tetrodotoxin::Ttx::Core::is_real("Real_32"_view));
  EXPECT_NOT(Tetrodotoxin::Ttx::Core::is_numeric("Real_32"_view));
  EXPECT(
      Tetrodotoxin::Ttx::Core::type_class("Real_32"_view) ==
      Tetrodotoxin::Ttx::Type::Class::Real);

  EXPECT(Tetrodotoxin::Ttx::Core::is_type("Bits_64"_view));
  EXPECT(Tetrodotoxin::Ttx::Core::is_numeric("Bits_64"_view));
  EXPECT(
      Tetrodotoxin::Ttx::Core::type_class("Bits_64"_view) ==
      Tetrodotoxin::Ttx::Type::Class::Integer);

  const Tetrodotoxin::Ttx::Type* boolean =
      Tetrodotoxin::Ttx::Core::find_type("Bool"_view);
  EXPECT(boolean != nullptr);
  EXPECT(boolean->get_class() == Tetrodotoxin::Ttx::Type::Class::Bool);
  EXPECT(boolean->get_layout() != nullptr);
  EXPECT(boolean->get_layout()->is_valid());
  EXPECT(boolean->get_layout()->is_concrete());
  EXPECT(boolean->get_layout()->is_terminal());
  EXPECT_EQ(boolean->get_layout()->get_byte_size(), 1);
  EXPECT_EQ(boolean->get_layout()->get_alignment(), 1);
}

PERIMORTEM_UNIT_TEST(TtxCore, describes_standard_aggregates) {
  const Tetrodotoxin::Ttx::Type* vec =
      Tetrodotoxin::Ttx::Core::find_type("Vec3D"_view);
  EXPECT(vec != nullptr);
  EXPECT(vec->is_aggregate());
  EXPECT(vec->get_class() == Tetrodotoxin::Ttx::Type::Class::Vector);
  EXPECT(vec->get_layout() != nullptr);
  EXPECT(vec->get_layout()->is_valid());
  EXPECT(vec->get_layout()->is_concrete());
  EXPECT_EQ(vec->get_layout()->get_entries().get_size(), 3);
  EXPECT_EQ(vec->get_layout()->get_byte_size(), 12);
  EXPECT_EQ(vec->get_layout()->get_alignment(), 16);
  EXPECT(
      Tetrodotoxin::Ttx::Core::type_class("Vec3D"_view) ==
      Tetrodotoxin::Ttx::Type::Class::Vector);

  const Tetrodotoxin::Ttx::Field* z =
      Tetrodotoxin::Ttx::Core::find_field("Vec3D"_view, "z"_view);
  EXPECT(z != nullptr);
  EXPECT_TEXT(z->get_type_name(), "Real_32"_view);
  EXPECT(z->get_layout() != nullptr);
  EXPECT(z->get_layout()->is_valid());
  EXPECT_EQ(z->get_layout()->get_byte_size(), 4);
  EXPECT_EQ(z->get_layout()->get_alignment(), 4);
  EXPECT_EQ(z->get_offset(), 8);
}

PERIMORTEM_UNIT_TEST(TtxCore, keeps_graphics_types_out_of_core) {
  EXPECT_NOT(Tetrodotoxin::Ttx::Core::is_type("Sampler_2D"_view));
  EXPECT_NOT(Tetrodotoxin::Ttx::Core::is_type("Color"_view));
  EXPECT(Tetrodotoxin::Ttx::Core::find_type("Color"_view) == nullptr);
}

PERIMORTEM_UNIT_TEST(TtxCore, returns_layout_facts_for_core_types) {
  Ttx::Layout scalar = Ttx::Core::type_layout(Ttx::TypeQuery("Real_32"_view));
  EXPECT(scalar.is_valid());
  EXPECT(scalar.is_concrete());
  EXPECT(scalar.is_terminal());
  EXPECT_EQ(scalar.get_byte_size(), 4);
  EXPECT_EQ(scalar.get_alignment(), 4);

  Ttx::Layout aggregate = Ttx::Core::type_layout(Ttx::TypeQuery("Vec4D"_view));
  EXPECT(aggregate.is_valid());
  EXPECT(aggregate.is_concrete());
  EXPECT_EQ(aggregate.get_entries().get_size(), 4);
  EXPECT_EQ(aggregate.get_byte_size(), 16);
  EXPECT_EQ(aggregate.get_alignment(), 16);

  const Ttx::Layout* bits_8 = Ttx::Core::find_layout("Bits_8"_view);
  ASSERT(bits_8);
  Ttx::TypeQuery vec("Vec"_view);
  vec.add_param(Ttx::TypeArgument::type("Bits_8"_view, *bits_8));
  vec.add_param(Ttx::TypeArgument::size_literal("16"_view, 16));

  Ttx::Layout generic = Ttx::Core::type_layout(vec);
  EXPECT(generic.is_valid());
  EXPECT_EQ(generic.get_byte_size(), 16);
  EXPECT_EQ(generic.get_alignment(), 1);

  Ttx::Layout missing = Ttx::Core::type_layout(Ttx::TypeQuery("Missing"_view));
  EXPECT_NOT(missing.is_valid());
}
