// Perimortem Engine
// Copyright © Matt Kaes

#include "ttx/type.hpp"

#include "validation/unit_test.hpp"

#include "perimortem/core/static/vector.hpp"

using namespace Perimortem::Core;
using namespace Validation;

static Harness TtxType = {
  .name = "TTX::Type"_view,
};

PERIMORTEM_UNIT_TEST(TtxType, alias_attrs) {
  static constexpr Static::Vector<Ttx::Attribute, 3> foreign_attributes = {{
    {"role"_view, "foreign"_view},
    {"abi"_view, "foreign"_view},
    {"cpp"_view, "ForeignCpp"_view},
  }};
  static constexpr Static::Vector<Ttx::Attribute, 2> alias_attributes = {{
    {"role"_view, "alias"_view},
    {"cpp"_view, "AliasCpp"_view},
  }};
  static constexpr Static::Vector<Ttx::Attribute, 2> middle_attributes = {{
    {"abi"_view, "middle"_view},
    {"cpp"_view, "MiddleCpp"_view},
  }};
  static constexpr Static::Vector<Ttx::Attribute, 1> relay_attributes = {{
    {"role"_view, "alias"_view},
  }};

  Ttx::Type foreign("Console"_view, foreign_attributes);
  Ttx::Type alias = Ttx::Type::alias(
      "Log"_view, foreign, Ttx::Documentation(), alias_attributes);
  Ttx::Type middle = Ttx::Type::alias(
      "ConsoleLog"_view, foreign, Ttx::Documentation(), middle_attributes);
  Ttx::Type relay = Ttx::Type::alias(
      "Relay"_view, middle, Ttx::Documentation(), relay_attributes);

  const Ttx::Attribute* role = alias.find_attribute("role"_view);
  ASSERT(role != nullptr);
  EXPECT_TEXT(role->get_value(), "alias"_view);

  EXPECT(alias.attribute_equals("role"_view, "alias"_view));
  EXPECT(alias.attribute_equals("role"_view, "foreign"_view));
  EXPECT(alias.attribute_equals("abi"_view, "foreign"_view));
  EXPECT_NOT(alias.attribute_equals("role"_view, "library"_view));

  const Ttx::Attribute* cpp = alias.resolve_attribute("cpp"_view);
  ASSERT(cpp != nullptr);
  EXPECT_TEXT(cpp->get_value(), "AliasCpp"_view);

  const Ttx::Attribute* abi = alias.resolve_attribute("abi"_view);
  ASSERT(abi != nullptr);
  EXPECT_TEXT(abi->get_value(), "foreign"_view);

  EXPECT(&relay.canonical() == &foreign);
  EXPECT(relay.attribute_equals("role"_view, "foreign"_view));
  EXPECT(relay.attribute_equals("abi"_view, "middle"_view));
  EXPECT(relay.attribute_equals("abi"_view, "foreign"_view));

  cpp = relay.resolve_attribute("cpp"_view);
  ASSERT(cpp != nullptr);
  EXPECT_TEXT(cpp->get_value(), "MiddleCpp"_view);
}

PERIMORTEM_UNIT_TEST(TtxType, describe) {
  static constexpr Static::Vector<Ttx::Attribute, 1> math_attributes = {{
    {Ttx::Type::display_name_attribute,
     "Perimortem.Math::Geometry::Size2D"_view},
  }};
  static constexpr Static::Vector<Ttx::Attribute, 1> graphics_attributes = {{
    {Ttx::Type::display_name_attribute, "Perimortem.Graphics::Size2D"_view},
  }};

  Ttx::Type local("Local"_view);
  EXPECT_TEXT(local.describe().get_view(), "Local"_view);

  Ttx::Type math_size("Size2D"_view, math_attributes);
  Ttx::Type graphics_size = Ttx::Type::alias(
      "Size2D"_view, math_size, Ttx::Documentation(), graphics_attributes);
  EXPECT_TEXT(
      graphics_size.describe().get_view(),
      "Perimortem.Graphics::Size2D alias of "
      "Perimortem.Math::Geometry::Size2D"_view);
}

PERIMORTEM_UNIT_TEST(TtxType, alias_lookup) {
  Ttx::Type bits("Bits_32"_view);
  Ttx::Type nested("Nested"_view);
  Static::Vector<Ttx::Member, 1> members = {{
    {"value"_view, bits},
  }};
  Static::Vector<const Ttx::Type*, 1> nested_types = {{
    &nested,
  }};
  Static::Vector<Ttx::Function, 1> functions = {{
    Ttx::Function("draw"_view, Ttx::Layout(), Ttx::Layout()),
  }};

  Ttx::Type root("Root"_view, members, nested_types, functions);
  Ttx::Type alias = Ttx::Type::alias("Alias"_view, root);

  const Ttx::Member* member = alias.find_member("value"_view);
  ASSERT(member != nullptr);
  EXPECT(&member->get_type() == &bits);
  EXPECT(alias.find_type("Nested"_view) == &nested);

  const Ttx::Function* function = alias.find_function("draw"_view);
  ASSERT(function != nullptr);
  EXPECT_TEXT(function->get_name(), "draw"_view);
}

PERIMORTEM_UNIT_TEST(TtxType, alias_docs) {
  static constexpr Static::Vector<View::Bytes, 1> root_lines = {{
    "Root docs"_view,
  }};
  static constexpr Static::Vector<View::Bytes, 1> alias_lines = {{
    "Alias docs"_view,
  }};

  Ttx::Type root("Root"_view, Ttx::Documentation(root_lines));
  Ttx::Type alias =
      Ttx::Type::alias("Alias"_view, root, Ttx::Documentation(alias_lines));

  EXPECT(&alias.canonical() == &root);
  EXPECT_TEXT(alias.get_documentation().line_at(0), "Alias docs"_view);
  ASSERT(!alias.canonical().is_invalid());
  EXPECT_TEXT(
      alias.canonical().get_documentation().line_at(0), "Root docs"_view);
}

PERIMORTEM_UNIT_TEST(TtxType, invalid) {
  Ttx::Type invalid(""_view);

  EXPECT(invalid.is_invalid());
  EXPECT_TEXT(invalid.get_name(), View::Bytes());
  EXPECT(invalid.canonical().is_invalid());
  EXPECT_NOT(invalid.equivalent_to(invalid));
}

PERIMORTEM_UNIT_TEST(TtxType, owned_layout) {
  Ttx::Type real("Real_32"_view);
  Static::Vector<Ttx::Member, 2> members = {{
    {"x"_view, real},
    {"y"_view, real},
  }};

  Ttx::Type point("Point"_view, Ttx::Layout(members));

  EXPECT_EQ(point.get_layout().get_member_count(), Count(2));
  EXPECT(point.get_members().get_data() == members.get_view().get_data());
  EXPECT(&point.get_layout().member_at(1).get_type() == &real);
}

PERIMORTEM_UNIT_TEST(TtxType, layout_cast) {
  Ttx::Type bits("Bits_32"_view);
  Static::Vector<Ttx::Member, 1> members = {{
    {"value"_view, bits},
  }};

  Ttx::Type storage("Storage"_view, Ttx::Layout(members));
  Ttx::Layout layout = storage;

  EXPECT(layout.equivalent_to(Ttx::Layout(members)));
}

PERIMORTEM_UNIT_TEST(TtxType, alias_shape) {
  Ttx::Type bits("Bits_32"_view);
  Static::Vector<Ttx::Member, 1> members = {{
    {"value"_view, bits},
  }};

  Ttx::Type storage("Storage"_view, Ttx::Layout(members));
  Ttx::Type alias = Ttx::Type::alias("Alias"_view, storage);

  EXPECT(alias.get_members().is_empty());
  EXPECT(Ttx::Layout(alias).equivalent_to(Ttx::Layout(storage)));
}

PERIMORTEM_UNIT_TEST(TtxType, alias_member) {
  Ttx::Type bits("Bits_32"_view);
  Static::Vector<Ttx::Member, 1> members = {{
    {"value"_view, bits},
  }};

  Ttx::Type storage("Storage"_view, Ttx::Layout(members));
  Ttx::Type alias = Ttx::Type::alias("Alias"_view, storage);

  const Ttx::Member* member = alias.find_member("value"_view);
  ASSERT(member != nullptr);
  EXPECT(&member->get_type() == &bits);
  EXPECT(alias.find_member("missing"_view) == nullptr);
}

PERIMORTEM_UNIT_TEST(TtxType, local_display) {
  Ttx::Type type("Local"_view);

  EXPECT_TEXT(type.describe().get_view(), "Local"_view);
}

PERIMORTEM_UNIT_TEST(TtxType, alias_display) {
  Ttx::Type root("Root"_view);
  Ttx::Type alias = Ttx::Type::alias("LocalRoot"_view, root);

  EXPECT_TEXT(alias.describe().get_view(), "LocalRoot alias of Root"_view);
}

PERIMORTEM_UNIT_TEST(TtxType, invalid_alias) {
  Ttx::Type invalid(""_view);
  Ttx::Type alias = Ttx::Type::alias("Broken"_view, invalid);

  EXPECT(alias.canonical().is_invalid());
  EXPECT_NOT(alias.equivalent_to(invalid));
  EXPECT(alias.find_member("value"_view) == nullptr);
  EXPECT(alias.find_type("Nested"_view) == nullptr);
  EXPECT(alias.find_function("call"_view) == nullptr);
}

PERIMORTEM_UNIT_TEST(TtxType, attr_override) {
  static constexpr Static::Vector<Ttx::Attribute, 1> root_attributes = {{
    {"cpp"_view, "RootCpp"_view},
  }};
  static constexpr Static::Vector<Ttx::Attribute, 1> alias_attributes = {{
    {"cpp"_view, "AliasCpp"_view},
  }};

  Ttx::Type root("Root"_view, root_attributes);
  Ttx::Type alias = Ttx::Type::alias(
      "Alias"_view, root, Ttx::Documentation(), alias_attributes);

  const Ttx::Attribute* attribute = alias.resolve_attribute("cpp"_view);
  ASSERT(attribute != nullptr);
  EXPECT_TEXT(attribute->get_value(), "AliasCpp"_view);
  EXPECT(alias.attribute_equals("cpp"_view, "RootCpp"_view));
  EXPECT(alias.attribute_equals("cpp"_view, "AliasCpp"_view));
}

PERIMORTEM_UNIT_TEST(TtxType, nested_missing) {
  Ttx::Type nested("Nested"_view);
  Static::Vector<const Ttx::Type*, 1> nested_types = {{
    &nested,
  }};

  Ttx::Type root("Root"_view, Ttx::Layout(), nested_types);

  EXPECT(root.find_type("Nested"_view) == &nested);
  EXPECT(root.find_type("Missing"_view) == nullptr);
  EXPECT(root.find_function("Missing"_view) == nullptr);
}

PERIMORTEM_UNIT_TEST(TtxType, layout_is_local) {
  Ttx::Type bits("Bits_32"_view);
  Static::Vector<Ttx::Member, 1> members = {{
    {"value"_view, bits},
  }};
  Ttx::Type storage("Storage"_view, Ttx::Layout(members));
  Ttx::Type alias = Ttx::Type::alias("Alias"_view, storage);

  EXPECT(alias.get_layout().is_empty());
  EXPECT_NOT(Ttx::Layout(alias).is_empty());
}

PERIMORTEM_UNIT_TEST(TtxType, attr_missing) {
  Ttx::Type type("Value"_view);

  EXPECT(type.find_attribute("missing"_view) == nullptr);
  EXPECT(type.resolve_attribute("missing"_view) == nullptr);
  EXPECT_NOT(type.attribute_equals("missing"_view, "value"_view));
}
