// Perimortem Engine
// Copyright © Matt Kaes

#include "ttx/attribute.hpp"

#include "validation/unit_test.hpp"

using namespace Perimortem::Core;
using namespace Validation;

static Harness TtxAttribute = {
  .name = "TTX::Attribute"_view,
};

PERIMORTEM_UNIT_TEST(TtxAttribute, empty) {
  Ttx::Attribute attribute;

  EXPECT(attribute.is_empty());
  EXPECT(attribute.get_key().is_empty());
  EXPECT(attribute.get_value().is_empty());
}

PERIMORTEM_UNIT_TEST(TtxAttribute, key_value) {
  Ttx::Attribute attribute("binding"_view, "uniform"_view);

  EXPECT_NOT(attribute.is_empty());
  EXPECT_TEXT(attribute.get_key(), "binding"_view);
  EXPECT_TEXT(attribute.get_value(), "uniform"_view);
}

PERIMORTEM_UNIT_TEST(TtxAttribute, key_only) {
  Ttx::Attribute attribute("required"_view, View::Bytes());

  EXPECT_NOT(attribute.is_empty());
  EXPECT_TEXT(attribute.get_key(), "required"_view);
  EXPECT(attribute.get_value().is_empty());
}

PERIMORTEM_UNIT_TEST(TtxAttribute, value_only) {
  Ttx::Attribute attribute(View::Bytes(), "Library"_view);

  EXPECT_NOT(attribute.is_empty());
  EXPECT(attribute.get_key().is_empty());
  EXPECT_TEXT(attribute.get_value(), "Library"_view);
}
