// Perimortem Engine
// Copyright © Matt Kaes

#include "validation/unit_test.hpp"

#include "perimortem/core/static/vector.hpp"

#include "ttx/type.hpp"

using namespace Perimortem::Core;
using namespace Validation;

static Harness TtxFunction = {
  .name = "TTX::Function"_view,
};

PERIMORTEM_UNIT_TEST(TtxFunction, empty) {
  Ttx::Function function;

  EXPECT(function.is_empty());
  EXPECT(function.get_name().is_empty());
}

PERIMORTEM_UNIT_TEST(TtxFunction, signature) {
  static constexpr Static::Vector<View::Bytes, 1> docs = {{
    "Draws the object."_view,
  }};
  Ttx::Type point("Point2D"_view);
  Ttx::Type color("Color"_view);
  Static::Vector<Ttx::Member, 1> parameters = {{
    {"position"_view, point},
  }};
  Static::Vector<Ttx::Member, 1> results = {{
    {View::Bytes(), color},
  }};

  Ttx::Function function(
      "draw"_view, Ttx::Layout(parameters), Ttx::Layout(results),
      Ttx::Documentation(docs));

  EXPECT_NOT(function.is_empty());
  EXPECT_TEXT(function.get_name(), "draw"_view);
  ASSERT_EQ(function.get_parameters().get_member_count(), Count(1));
  ASSERT_EQ(function.get_result().get_member_count(), Count(1));
  EXPECT(&function.get_parameters().member_at(0).get_type() == &point);
  EXPECT(&function.get_result().member_at(0).get_type() == &color);
  EXPECT_TEXT(
      function.get_documentation().line_at(0), "Draws the object."_view);
}

PERIMORTEM_UNIT_TEST(TtxFunction, alias_params) {
  Ttx::Type point("Point2D"_view);
  Ttx::Type local_point = Ttx::Type::alias("LocalPoint"_view, point);
  Static::Vector<Ttx::Member, 1> parameters = {{
    {"position"_view, local_point},
  }};
  Static::Vector<Ttx::Member, 1> target = {{
    {"position"_view, point},
  }};

  Ttx::Function function("move"_view, Ttx::Layout(parameters), Ttx::Layout());

  EXPECT(function.get_parameters().fits(Ttx::Layout(target)));
}

PERIMORTEM_UNIT_TEST(TtxFunction, default_params) {
  Ttx::Type real("Real_32"_view);
  Static::Vector<Ttx::Member, 1> source = {{
    {"x"_view, real},
  }};
  Static::Vector<Ttx::Member, 2> parameters = {{
    {"x"_view, real},
    {"y"_view, real, True},
  }};

  Ttx::Function function("point"_view, Ttx::Layout(parameters), Ttx::Layout());

  EXPECT(Ttx::Layout(source).fits(function.get_parameters()));
}

PERIMORTEM_UNIT_TEST(TtxFunction, empty_result) {
  Ttx::Function function("tick"_view, Ttx::Layout(), Ttx::Layout());

  EXPECT_NOT(function.is_empty());
  EXPECT(function.get_result().is_empty());
  EXPECT(Ttx::Layout().equivalent_to(function.get_result()));
}

PERIMORTEM_UNIT_TEST(TtxFunction, docs) {
  static constexpr Static::Vector<View::Bytes, 1> docs = {{
    "Runs the function."_view,
  }};

  Ttx::Function function(
      "run"_view, Ttx::Layout(), Ttx::Layout(), Ttx::Documentation(docs));

  EXPECT_TEXT(
      function.get_documentation().line_at(0), "Runs the function."_view);
}
