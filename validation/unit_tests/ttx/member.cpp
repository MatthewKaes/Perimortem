// Perimortem Engine
// Copyright © Matt Kaes

#include "validation/unit_test.hpp"

#include "perimortem/core/static/vector.hpp"

#include "ttx/type.hpp"

using namespace Perimortem::Core;
using namespace Validation;

static Harness TtxMember = {
  .name = "TTX::Member"_view,
};

PERIMORTEM_UNIT_TEST(TtxMember, shape) {
  static constexpr Static::Vector<View::Bytes, 1> docs = {{
    "Coordinate"_view,
  }};

  Ttx::Type real("Real_32"_view);
  Ttx::Member member("x"_view, real, True, Ttx::Documentation(docs));

  EXPECT(member.is_named());
  EXPECT(member.is_defaulted());
  EXPECT(&member.get_type() == &real);
  EXPECT_TEXT(member.get_documentation().line_at(0), "Coordinate"_view);
}

PERIMORTEM_UNIT_TEST(TtxMember, equivalence) {
  Ttx::Type real("Real_32"_view);
  Ttx::Type alias = Ttx::Type::alias("RealAlias"_view, real);
  Ttx::Type bits("Bits_32"_view);

  EXPECT(
      Ttx::Member("x"_view, alias)
          .equivalent_to(Ttx::Member("y"_view, real, True)));
  EXPECT_NOT(
      Ttx::Member("x"_view, real).equivalent_to(Ttx::Member("x"_view, bits)));
}

PERIMORTEM_UNIT_TEST(TtxMember, attributes) {
  Ttx::Type real("Real_32"_view);
  Static::Vector<Ttx::Attribute, 1> attributes = {{
    {"builtin"_view, "position"_view},
  }};
  Ttx::Member member(
      "position"_view, real, False, Ttx::Documentation(), attributes);

  ASSERT_EQ(member.get_attributes().get_size(), Count(1));
  EXPECT_TEXT(member.get_attributes()[0].get_key(), "builtin"_view);
  EXPECT_TEXT(member.get_attributes()[0].get_value(), "position"_view);
}

PERIMORTEM_UNIT_TEST(TtxMember, unnamed) {
  Ttx::Type real("Real_32"_view);
  Ttx::Member member(View::Bytes(), real);

  EXPECT_NOT(member.is_named());
  EXPECT(&member.get_type() == &real);
}

PERIMORTEM_UNIT_TEST(TtxMember, docs_ignored) {
  static constexpr Static::Vector<View::Bytes, 1> left_docs = {{
    "Left"_view,
  }};
  static constexpr Static::Vector<View::Bytes, 1> right_docs = {{
    "Right"_view,
  }};
  Ttx::Type real("Real_32"_view);

  EXPECT(
      Ttx::Member("x"_view, real, Ttx::Documentation(left_docs))
          .equivalent_to(
              Ttx::Member("x"_view, real, Ttx::Documentation(right_docs))));
}

PERIMORTEM_UNIT_TEST(TtxMember, defaults_ignored) {
  Ttx::Type real("Real_32"_view);

  EXPECT(
      Ttx::Member("x"_view, real)
          .equivalent_to(Ttx::Member("x"_view, real, True)));
}

PERIMORTEM_UNIT_TEST(TtxMember, alias_chain) {
  Ttx::Type real("Real_32"_view);
  Ttx::Type local = Ttx::Type::alias("LocalReal"_view, real);
  Ttx::Type public_real = Ttx::Type::alias("PublicReal"_view, local);

  EXPECT(
      Ttx::Member("x"_view, public_real)
          .equivalent_to(Ttx::Member("x"_view, real)));
}
