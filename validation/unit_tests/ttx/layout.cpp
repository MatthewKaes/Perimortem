// Perimortem Engine
// Copyright © Matt Kaes

#include "ttx/layout.hpp"

#include "validation/unit_test.hpp"

#include "perimortem/core/static/vector.hpp"

#include "ttx/type.hpp"

using namespace Perimortem::Core;
using namespace Validation;

static Harness TtxLayout = {
  .name = "TTX::Layout"_view,
};

PERIMORTEM_UNIT_TEST(TtxLayout, leftmost_name) {
  Ttx::Type real("Real_32"_view);
  Ttx::Type bits("Bits_32"_view);
  static constexpr View::Bytes x = "x"_view;

  Static::Vector<Ttx::Member, 3> members = {{
    {x, real},
    {"y"_view, bits},
    {x, bits},
  }};

  Ttx::Layout layout(members);
  const Ttx::Member* member = layout.find_member(x);
  ASSERT(member != nullptr);
  EXPECT(&member->get_type() == &real);
}

PERIMORTEM_UNIT_TEST(TtxLayout, exact_shape) {
  Ttx::Type real("Real_32"_view);
  Ttx::Type bits("Bits_32"_view);
  Ttx::Type real_alias = Ttx::Type::alias("RealAlias"_view, real);

  Static::Vector<Ttx::Member, 2> left = {{
    {"x"_view, real_alias, True},
    {"y"_view, bits},
  }};
  Static::Vector<Ttx::Member, 2> same = {{
    {"x"_view, real},
    {"y"_view, bits, True},
  }};
  Static::Vector<Ttx::Member, 2> reordered = {{
    {"y"_view, bits},
    {"x"_view, real},
  }};

  EXPECT(Ttx::Layout(left).equivalent_to(Ttx::Layout(same)));
  EXPECT_NOT(Ttx::Layout(left).equivalent_to(Ttx::Layout(reordered)));
}

PERIMORTEM_UNIT_TEST(TtxLayout, named_fit) {
  Ttx::Type real("Real_32"_view);
  Ttx::Type bits("Bits_32"_view);

  Static::Vector<Ttx::Member, 3> target = {{
    {"x"_view, real},
    {"y"_view, bits},
    {"z"_view, real, True},
  }};
  Static::Vector<Ttx::Member, 2> source = {{
    {"y"_view, bits},
    {"x"_view, real},
  }};
  Static::Vector<Ttx::Member, 2> missing = {{
    {"x"_view, real},
    {"z"_view, real},
  }};

  EXPECT(Ttx::Layout(source).fits(Ttx::Layout(target)));
  EXPECT_NOT(Ttx::Layout(missing).fits(Ttx::Layout(target)));
  EXPECT_EQ(
      Ttx::Layout(source).source_index_for(Ttx::Layout(target), 0), Count(1));
  EXPECT_EQ(
      Ttx::Layout(source).source_index_for(Ttx::Layout(target), 1), Count(0));
  EXPECT_EQ(
      Ttx::Layout(source).source_index_for(Ttx::Layout(target), 2), Count(-1));
}

PERIMORTEM_UNIT_TEST(TtxLayout, trailing_defaults) {
  Ttx::Type real("Real_32"_view);
  Ttx::Type bits("Bits_32"_view);

  Static::Vector<Ttx::Member, 1> source = {{
    {View::Bytes(), real},
  }};
  Static::Vector<Ttx::Member, 2> target = {{
    {View::Bytes(), real},
    {View::Bytes(), bits, True},
  }};
  Static::Vector<Ttx::Member, 2> default_gap = {{
    {View::Bytes(), real, True},
    {View::Bytes(), bits},
  }};

  EXPECT(Ttx::Layout(source).fits(Ttx::Layout(target)));
  EXPECT_NOT(Ttx::Layout(source).fits(Ttx::Layout(default_gap)));
}

PERIMORTEM_UNIT_TEST(TtxLayout, alias_projection) {
  Ttx::Type real("Real_32"_view);
  Static::Vector<Ttx::Member, 1> members = {{
    {"x"_view, real},
  }};

  Ttx::Type point("Point"_view, members);
  Ttx::Type point_alias = Ttx::Type::alias("PointAlias"_view, point);

  EXPECT(Ttx::Layout(point_alias).equivalent_to(Ttx::Layout(point)));
}

PERIMORTEM_UNIT_TEST(TtxLayout, empty_target) {
  Ttx::Type real("Real_32"_view);
  Static::Vector<Ttx::Member, 1> members = {{
    {"x"_view, real},
  }};

  EXPECT(Ttx::Layout().fits(Ttx::Layout()));
  EXPECT_NOT(Ttx::Layout(members).fits(Ttx::Layout()));
}

PERIMORTEM_UNIT_TEST(TtxLayout, duplicate_order) {
  Ttx::Type real("Real_32"_view);
  Ttx::Type bits("Bits_32"_view);
  Static::Vector<Ttx::Member, 2> source = {{
    {"x"_view, real},
    {"x"_view, bits},
  }};
  Static::Vector<Ttx::Member, 2> same = {{
    {"x"_view, real},
    {"x"_view, bits},
  }};
  Static::Vector<Ttx::Member, 2> swapped = {{
    {"x"_view, bits},
    {"x"_view, real},
  }};

  EXPECT(Ttx::Layout(source).fits(Ttx::Layout(same)));
  EXPECT_NOT(Ttx::Layout(source).fits(Ttx::Layout(swapped)));
}

PERIMORTEM_UNIT_TEST(TtxLayout, mixed_names) {
  Ttx::Type real("Real_32"_view);
  Static::Vector<Ttx::Member, 2> source = {{
    {"x"_view, real},
    {View::Bytes(), real},
  }};
  Static::Vector<Ttx::Member, 2> target = {{
    {"x"_view, real},
    {"y"_view, real},
  }};

  EXPECT_NOT(Ttx::Layout(source).fits(Ttx::Layout(target)));
}

PERIMORTEM_UNIT_TEST(TtxLayout, source_overrun) {
  Ttx::Type real("Real_32"_view);
  Static::Vector<Ttx::Member, 2> source = {{
    {View::Bytes(), real},
    {View::Bytes(), real},
  }};
  Static::Vector<Ttx::Member, 1> target = {{
    {View::Bytes(), real},
  }};

  EXPECT_NOT(Ttx::Layout(source).fits(Ttx::Layout(target)));
}

PERIMORTEM_UNIT_TEST(TtxLayout, alias_fit) {
  Ttx::Type real("Real_32"_view);
  Ttx::Type real_alias = Ttx::Type::alias("RealAlias"_view, real);
  Static::Vector<Ttx::Member, 1> source = {{
    {"x"_view, real_alias},
  }};
  Static::Vector<Ttx::Member, 1> target = {{
    {"x"_view, real},
  }};

  EXPECT(Ttx::Layout(source).fits(Ttx::Layout(target)));
}

PERIMORTEM_UNIT_TEST(TtxLayout, name_mismatch) {
  Ttx::Type real("Real_32"_view);
  Static::Vector<Ttx::Member, 1> source = {{
    {"x"_view, real},
  }};
  Static::Vector<Ttx::Member, 1> target = {{
    {"y"_view, real},
  }};

  EXPECT_NOT(Ttx::Layout(source).fits(Ttx::Layout(target)));
}

PERIMORTEM_UNIT_TEST(TtxLayout, source_extra) {
  Ttx::Type real("Real_32"_view);
  Static::Vector<Ttx::Member, 2> source = {{
    {"x"_view, real},
    {"z"_view, real},
  }};
  Static::Vector<Ttx::Member, 1> target = {{
    {"x"_view, real},
  }};

  EXPECT_NOT(Ttx::Layout(source).fits(Ttx::Layout(target)));
}

PERIMORTEM_UNIT_TEST(TtxLayout, named_default) {
  Ttx::Type real("Real_32"_view);
  Static::Vector<Ttx::Member, 1> source = {{
    {"x"_view, real},
  }};
  Static::Vector<Ttx::Member, 2> target = {{
    {"x"_view, real},
    {"y"_view, real, True},
  }};

  EXPECT(Ttx::Layout(source).fits(Ttx::Layout(target)));
}

PERIMORTEM_UNIT_TEST(TtxLayout, name_not_equal) {
  Ttx::Type real("Real_32"_view);
  Static::Vector<Ttx::Member, 1> left = {{
    {"x"_view, real},
  }};
  Static::Vector<Ttx::Member, 1> right = {{
    {"y"_view, real},
  }};

  EXPECT_NOT(Ttx::Layout(left).equivalent_to(Ttx::Layout(right)));
}

PERIMORTEM_UNIT_TEST(TtxLayout, positional_fit) {
  Ttx::Type real("Real_32"_view);
  Static::Vector<Ttx::Member, 1> source = {{
    {View::Bytes(), real},
  }};
  Static::Vector<Ttx::Member, 1> target = {{
    {View::Bytes(), real},
  }};

  EXPECT(Ttx::Layout(source).fits(Ttx::Layout(target)));
}
