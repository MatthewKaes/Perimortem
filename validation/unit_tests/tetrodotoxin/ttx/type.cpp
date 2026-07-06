// Perimortem Engine
// Copyright © Matt Kaes

#include "validation/unit_test.hpp"

#include "perimortem/core/static/vector.hpp"

#include "ttx/type.hpp"

using namespace Perimortem::Core;
using namespace Validation;

static Harness TtxType = {
  .name = "TTX::Type"_view,
};

PERIMORTEM_UNIT_TEST(TtxType, alias_attrs) {
  static constexpr Static::Vector<Ttx::Attribute, 3> foreign_attributes = {{
    {"isa"_view, "Foreign"_view},
    {"abi"_view, "foreign"_view},
    {"cpp"_view, "ForeignCpp"_view},
  }};
  static constexpr Static::Vector<Ttx::Attribute, 2> alias_attributes = {{
    {"isa"_view, "Alias"_view},
    {"cpp"_view, "AliasCpp"_view},
  }};
  static constexpr Static::Vector<Ttx::Attribute, 2> middle_attributes = {{
    {"abi"_view, "middle"_view},
    {"cpp"_view, "MiddleCpp"_view},
  }};
  static constexpr Static::Vector<Ttx::Attribute, 1> relay_attributes = {{
    {"isa"_view, "Alias"_view},
  }};

  Ttx::Type foreign("Console"_view, foreign_attributes);
  Ttx::Type alias = Ttx::Type::alias(
      "Log"_view, foreign, Ttx::Documentation(), alias_attributes);
  Ttx::Type middle = Ttx::Type::alias(
      "ConsoleLog"_view, foreign, Ttx::Documentation(), middle_attributes);
  Ttx::Type relay = Ttx::Type::alias(
      "Relay"_view, middle, Ttx::Documentation(), relay_attributes);

  const Ttx::Attribute* isa = alias.find_attribute("isa"_view);
  ASSERT(isa != nullptr);
  EXPECT_TEXT(isa->get_value(), "Alias"_view);

  EXPECT(alias.attribute_equals("isa"_view, "Alias"_view));
  EXPECT(alias.attribute_equals("isa"_view, "Foreign"_view));
  EXPECT(alias.attribute_equals("abi"_view, "foreign"_view));
  EXPECT_NOT(alias.attribute_equals("isa"_view, "Library"_view));

  const Ttx::Attribute* cpp = alias.resolve_attribute("cpp"_view);
  ASSERT(cpp != nullptr);
  EXPECT_TEXT(cpp->get_value(), "AliasCpp"_view);

  const Ttx::Attribute* abi = alias.resolve_attribute("abi"_view);
  ASSERT(abi != nullptr);
  EXPECT_TEXT(abi->get_value(), "foreign"_view);

  EXPECT(relay.canonical() == &foreign);
  EXPECT(relay.attribute_equals("isa"_view, "Foreign"_view));
  EXPECT(relay.attribute_equals("abi"_view, "middle"_view));
  EXPECT(relay.attribute_equals("abi"_view, "foreign"_view));

  cpp = relay.resolve_attribute("cpp"_view);
  ASSERT(cpp != nullptr);
  EXPECT_TEXT(cpp->get_value(), "MiddleCpp"_view);
}
